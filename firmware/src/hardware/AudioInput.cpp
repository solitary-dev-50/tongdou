#include "hardware/AudioInput.h"

#include <driver/i2s.h>
#include <esp_err.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kSampleCount = 128;
constexpr unsigned long kLogIntervalMs = 1000;

int32_t sampleMagnitude(int32_t raw) {
  // INMP441 sends 24-bit audio inside a 32-bit I2S slot.
  const int32_t sample = raw >> 8;
  return sample < 0 ? -sample : sample;
}

}  // namespace

void AudioInput::begin() {
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = kSampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = tongdou::pins::I2S_MIC_BCLK;
  pins.ws_io_num = tongdou::pins::I2S_MIC_LRCLK;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = tongdou::pins::I2S_MIC_DATA;

  esp_err_t err = i2s_driver_install(kMicPort, &config, 0, nullptr);
  if (err == ESP_OK) {
    err = i2s_set_pin(kMicPort, &pins);
  }

  if (err == ESP_OK) {
    i2s_zero_dma_buffer(kMicPort);
    ready_ = true;
  }

  Serial.print("audio input pins bclk=");
  Serial.print(static_cast<int>(tongdou::pins::I2S_MIC_BCLK));
  Serial.print(" lrclk=");
  Serial.print(static_cast<int>(tongdou::pins::I2S_MIC_LRCLK));
  Serial.print(" data=");
  Serial.print(static_cast<int>(tongdou::pins::I2S_MIC_DATA));
  Serial.print(" status=");
  Serial.println(ready_ ? "ready" : esp_err_to_name(err));
}

void AudioInput::update() {
  const unsigned long now = millis();
  if (now - lastLogMs_ < kLogIntervalMs) {
    return;
  }

  lastLogMs_ = now;
  const AudioInputSnapshot snapshot = readLevel();

  Serial.print("audio input ready=");
  Serial.print(snapshot.ready ? "1" : "0");
  Serial.print(" samples=");
  Serial.print(snapshot.samplesRead);
  Serial.print(" peak=");
  Serial.print(snapshot.peak);
  Serial.print(" avg_abs=");
  Serial.println(snapshot.averageAbs);
}

AudioInputSnapshot AudioInput::readLevel() {
  AudioInputSnapshot snapshot;
  snapshot.ready = ready_;
  if (!ready_) {
    return snapshot;
  }

  int32_t samples[kSampleCount] = {};
  size_t bytesRead = 0;
  const esp_err_t err = i2s_read(kMicPort, samples, sizeof(samples), &bytesRead, 0);
  if (err != ESP_OK || bytesRead == 0) {
    return snapshot;
  }

  snapshot.samplesRead = bytesRead / sizeof(samples[0]);
  snapshot.hasSamples = snapshot.samplesRead > 0;

  int64_t sum = 0;
  int32_t peak = 0;
  for (size_t i = 0; i < snapshot.samplesRead; ++i) {
    const int32_t magnitude = sampleMagnitude(samples[i]);
    sum += magnitude;
    if (magnitude > peak) {
      peak = magnitude;
    }
  }

  snapshot.peak = peak;
  snapshot.averageAbs = static_cast<int32_t>(sum / static_cast<int64_t>(snapshot.samplesRead));
  return snapshot;
}

bool AudioInput::ready() const {
  return ready_;
}

}  // namespace tongdou
