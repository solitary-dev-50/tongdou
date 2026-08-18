#include "hardware/AudioOutput.h"

#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <driver/i2s.h>
#include <esp_err.h>
#include <math.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr i2s_port_t kSpeakerPort = I2S_NUM_1;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kFramesPerChunk = 64;
constexpr size_t kPcmReadSamples = 1024;
constexpr size_t kPcmReadBytes = kPcmReadSamples * sizeof(int16_t);
constexpr uint32_t kPlaybackTaskStack = 8192;
constexpr UBaseType_t kPlaybackTaskPriority = 1;
constexpr const char* kLittleFsPartitionLabel = "littlefs";
constexpr const char* kFirstSummonAudioPath = "/audio/demo_00_first_summon.pcm";
constexpr const char* kAudioPrefsNamespace = "td_audio";
constexpr const char* kVolumePercentKey = "volume";
constexpr bool kAudioClipDebug = false;
constexpr float kPi = 3.14159265358979323846F;
constexpr int16_t kToneAmplitude = 10000;
uint8_t gPcmPlaybackBuffer[kPcmReadBytes] = {};

int16_t scaleSample(int16_t sample, uint8_t volumePercent) {
  const int32_t scaled =
      static_cast<int32_t>(sample) * static_cast<int32_t>(volumePercent) / 100;
  return static_cast<int16_t>(constrain(scaled, -32768, 32767));
}

}  // namespace

void AudioOutput::begin() {
  Preferences prefs;
  if (prefs.begin(kAudioPrefsNamespace, true)) {
    volumePercent_ =
        static_cast<uint8_t>(constrain(prefs.getUChar(kVolumePercentKey, volumePercent_),
                                       0, 100));
    prefs.end();
  }

  digitalWrite(tongdou::pins::SPK_CTRL, HIGH);
  pinMode(tongdou::pins::SPK_CTRL, OUTPUT);

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = kSampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = tongdou::pins::I2S_SPK_BCLK;
  pins.ws_io_num = tongdou::pins::I2S_SPK_LRCLK;
  pins.data_out_num = tongdou::pins::I2S_SPK_DATA;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  esp_err_t err = i2s_driver_install(kSpeakerPort, &config, 0, nullptr);
  if (err == ESP_OK) {
    err = i2s_set_pin(kSpeakerPort, &pins);
  }

  if (err == ESP_OK) {
    i2s_zero_dma_buffer(kSpeakerPort);
    ready_ = true;
    currentSampleRate_ = kSampleRate;
  }

  fsReady_ = LittleFS.begin(false, "/littlefs", 10, kLittleFsPartitionLabel);
  if (fsReady_) {
    Serial.print(F("audio fs littlefs mounted total="));
    Serial.print(LittleFS.totalBytes());
    Serial.print(F(" used="));
    Serial.println(LittleFS.usedBytes());
    if (!LittleFS.exists(kFirstSummonAudioPath)) {
      Serial.print(F("audio fs missing "));
      Serial.println(kFirstSummonAudioPath);
      Serial.println(F("audio fs hint: run `pio run -t uploadfs` from local_demo_firmware/firmware"));
    }
  } else {
    Serial.println(F("audio fs littlefs mount failed"));
    Serial.println(F("audio fs hint: run `pio run -t uploadfs` from local_demo_firmware/firmware"));
  }
}

void AudioOutput::playTestTone(uint16_t frequencyHz, uint16_t durationMs) {
  if (!ready_ || frequencyHz == 0 || durationMs == 0) {
    return;
  }

  const uint32_t totalFrames = (static_cast<uint32_t>(durationMs) * kSampleRate) / 1000U;
  uint32_t writtenFrames = 0;
  float phase = 0.0F;
  const float phaseStep = (2.0F * kPi * static_cast<float>(frequencyHz)) /
                          static_cast<float>(kSampleRate);

  int16_t samples[kFramesPerChunk * 2] = {};
  while (writtenFrames < totalFrames) {
    const uint32_t framesThisChunk =
        min<uint32_t>(kFramesPerChunk, totalFrames - writtenFrames);

    for (uint32_t frame = 0; frame < framesThisChunk; ++frame) {
      const int16_t sample = static_cast<int16_t>(sinf(phase) * kToneAmplitude);
      const int16_t scaledSample = scaleSample(sample, volumePercent_);
      samples[frame * 2] = scaledSample;
      samples[frame * 2 + 1] = scaledSample;
      phase += phaseStep;
      if (phase > 2.0F * kPi) {
        phase -= 2.0F * kPi;
      }
    }

    size_t bytesWritten = 0;
    i2s_write(kSpeakerPort, samples, framesThisChunk * 2 * sizeof(samples[0]),
              &bytesWritten, portMAX_DELAY);
    writtenFrames += framesThisChunk;
  }

  i2s_zero_dma_buffer(kSpeakerPort);
}

void AudioOutput::playClip(uint16_t clipId, const char* text) {
  playClip(clipId, "", text);
}

void AudioOutput::playClip(uint16_t clipId, const char* fileName, const char* text) {
  if (clipId == 0) {
    return;
  }

  if (kAudioClipDebug) {
    Serial.print("audio clip=");
    Serial.print(clipId);
    Serial.print(" file=");
    Serial.print(fileName == nullptr ? "" : fileName);
    Serial.print(" text=");
    Serial.println(text == nullptr ? "" : text);
  }

  if (fileName == nullptr || fileName[0] == '\0') {
    Serial.println(F("audio playback skipped: empty file"));
    return;
  }

  startPlaybackTask(fileName);
}

bool AudioOutput::startPlaybackTask(const char* fileName) {
  if (!ready_) {
    Serial.println(F("audio playback failed: output not ready"));
    return false;
  }

  if (!fsReady_) {
    Serial.println(F("audio playback failed: littlefs not mounted"));
    return false;
  }

  if (playbackRunning_ || playbackTask_ != nullptr) {
    Serial.println(F("audio playback stopping previous clip"));
    stopRequested_ = true;
    i2s_zero_dma_buffer(kSpeakerPort);
    const unsigned long waitUntilMs = millis() + 180;
    while ((playbackRunning_ || playbackTask_ != nullptr) &&
           static_cast<long>(millis() - waitUntilMs) < 0) {
      delay(1);
    }
    if (playbackRunning_ || playbackTask_ != nullptr) {
      Serial.println(F("audio playback skipped: previous clip still stopping"));
      return false;
    }
  }

  strlcpy(playbackFileName_, fileName, sizeof(playbackFileName_));
  stopRequested_ = false;
  playbackStartedAtMs_ = 0;
  playbackStartPending_ = true;
  playbackRunning_ = true;

  const BaseType_t created =
      xTaskCreate(playbackTaskEntry, "audio_pcm_play", kPlaybackTaskStack, this,
                  kPlaybackTaskPriority, &playbackTask_);
  if (created != pdPASS) {
    playbackTask_ = nullptr;
    playbackRunning_ = false;
    Serial.println(F("audio playback failed: task create failed"));
    return false;
  }

  return true;
}

void AudioOutput::playbackTaskEntry(void* context) {
  static_cast<AudioOutput*>(context)->playbackTask();
  vTaskDelete(nullptr);
}

void AudioOutput::playbackTask() {
  const char* fileName = playbackFileName_;
  Serial.print(F("audio playback start file="));
  Serial.println(fileName);

  File file = LittleFS.open(fileName, "r");
  if (!file) {
    Serial.print(F("audio playback failed: file not found "));
    Serial.println(fileName);
    playbackTask_ = nullptr;
    playbackRunning_ = false;
    return;
  }

  bool ok = true;
  size_t totalBytes = 0;

  while (!stopRequested_ && file.available()) {
    const size_t bytesRead = file.read(gPcmPlaybackBuffer, sizeof(gPcmPlaybackBuffer));
    if (bytesRead == 0) {
      if (file.available()) {
        Serial.println(F("audio playback failed: read returned 0"));
        ok = false;
      }
      break;
    }

    if ((bytesRead % sizeof(int16_t)) != 0) {
      Serial.println(F("audio playback failed: odd pcm byte count"));
      ok = false;
      break;
    }

    totalBytes += bytesRead;
    if (!writePcm16Mono(reinterpret_cast<const int16_t*>(gPcmPlaybackBuffer),
                        bytesRead / sizeof(int16_t), kSampleRate)) {
      Serial.println(F("audio playback failed: i2s write failed"));
      ok = false;
      break;
    }
  }

  file.close();
  i2s_zero_dma_buffer(kSpeakerPort);

  if (stopRequested_) {
    Serial.print(F("audio playback stopped file="));
  } else if (ok) {
    Serial.print(F("audio playback complete file="));
  } else {
    Serial.print(F("audio playback aborted file="));
  }
  Serial.print(fileName);
  Serial.print(F(" bytes="));
  Serial.println(totalBytes);

  playbackTask_ = nullptr;
  playbackRunning_ = false;
  playbackStartPending_ = false;
}

bool AudioOutput::writePcm16Mono(const int16_t* samples, size_t sampleCount,
                                 uint32_t sampleRate) {
  if (!ready_ || samples == nullptr || sampleCount == 0 || sampleRate == 0) {
    return false;
  }

  if (currentSampleRate_ != sampleRate) {
    const esp_err_t err = i2s_set_sample_rates(kSpeakerPort, sampleRate);
    if (err != ESP_OK) {
      return false;
    }
    currentSampleRate_ = sampleRate;
  }

  size_t offset = 0;
  int16_t stereo[kFramesPerChunk * 2] = {};
  while (offset < sampleCount) {
    const size_t framesThisChunk = min(kFramesPerChunk, sampleCount - offset);
    for (size_t frame = 0; frame < framesThisChunk; ++frame) {
      const int16_t sample = scaleSample(samples[offset + frame], volumePercent_);
      stereo[frame * 2] = sample;
      stereo[frame * 2 + 1] = sample;
    }

    size_t bytesWritten = 0;
    const esp_err_t err =
        i2s_write(kSpeakerPort, stereo, framesThisChunk * 2 * sizeof(stereo[0]),
                  &bytesWritten, portMAX_DELAY);
    if (err != ESP_OK || bytesWritten == 0) {
      return false;
    }
    if (playbackStartPending_) {
      playbackStartedAtMs_ = millis();
      playbackStartPending_ = false;
      Serial.print(F("audio pcm t0 ms="));
      Serial.println(playbackStartedAtMs_);
    }
    offset += framesThisChunk;
  }

  return true;
}

void AudioOutput::stopStream() {
  if (!ready_) {
    return;
  }

  stopRequested_ = true;
  playbackStartPending_ = false;
  i2s_zero_dma_buffer(kSpeakerPort);
}

bool AudioOutput::ready() const {
  return ready_;
}

uint32_t AudioOutput::lastPlaybackStartedAtMs() const {
  return playbackStartedAtMs_;
}

uint8_t AudioOutput::volumePercent() const {
  return volumePercent_;
}

void AudioOutput::setVolumePercent(uint8_t percent) {
  volumePercent_ = static_cast<uint8_t>(constrain(percent, 0, 100));

  Preferences prefs;
  if (!prefs.begin(kAudioPrefsNamespace, false)) {
    Serial.println(F("audio volume save failed: preferences open failed"));
    return;
  }
  prefs.putUChar(kVolumePercentKey, volumePercent_);
  prefs.end();

  Serial.print(F("audio volume saved percent="));
  Serial.println(volumePercent_);
}

}  // namespace tongdou
