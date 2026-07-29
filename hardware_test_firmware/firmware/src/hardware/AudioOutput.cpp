#include "hardware/AudioOutput.h"

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
constexpr const char* kAudioPrefsNamespace = "td_audio";
constexpr const char* kVolumePercentKey = "volume";
constexpr float kPi = 3.14159265358979323846F;
constexpr int16_t kToneAmplitude = 10000;

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

  Serial.println(ready_ ? F("audio output ready: test tone only")
                        : F("audio output failed"));
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

bool AudioOutput::ready() const {
  return ready_;
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
