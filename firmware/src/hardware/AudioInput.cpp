#include "hardware/AudioInput.h"

#include <driver/i2s.h>
#include <esp_err.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kSampleCount = 128;

int32_t abs32(int32_t value) {
  return value < 0 ? -value : value;
}

int16_t sampleToPcm16(int32_t raw) {
  return static_cast<int16_t>(raw);
}

}  // namespace

void AudioInput::begin() {
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  config.sample_rate = kSampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
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
  pins.bck_io_num = tongdou::pins::PDM_MIC_CLK;
  pins.ws_io_num = I2S_PIN_NO_CHANGE;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = tongdou::pins::PDM_MIC_DATA;

  esp_err_t err = i2s_driver_install(kMicPort, &config, 0, nullptr);
  if (err == ESP_OK) {
    err = i2s_set_pin(kMicPort, &pins);
  }
  if (err == ESP_OK) {
    err = i2s_set_pdm_rx_down_sample(kMicPort, I2S_PDM_DSR_8S);
  }

  if (err == ESP_OK) {
    i2s_zero_dma_buffer(kMicPort);
    ready_ = true;
  }

}

void AudioInput::update() {
}

AudioInputSnapshot AudioInput::readLevel() {
  AudioInputSnapshot snapshot;
  snapshot.ready = ready_;
  if (!ready_) {
    return snapshot;
  }

  int16_t samples[kSampleCount] = {};
  size_t bytesRead = 0;
  const esp_err_t err = i2s_read(kMicPort, samples, sizeof(samples), &bytesRead, 0);
  if (err != ESP_OK || bytesRead == 0) {
    return snapshot;
  }

  snapshot.samplesRead = bytesRead / sizeof(samples[0]);
  snapshot.hasSamples = snapshot.samplesRead > 0;

  int64_t rawSum = 0;
  int32_t minSample = samples[0];
  int32_t maxSample = samples[0];
  for (size_t i = 0; i < snapshot.samplesRead; ++i) {
    const int32_t sample = samples[i];
    rawSum += sample;
    if (sample < minSample) {
      minSample = sample;
    }
    if (sample > maxSample) {
      maxSample = sample;
    }
  }

  const int32_t mean = static_cast<int32_t>(rawSum / static_cast<int64_t>(snapshot.samplesRead));
  int64_t centeredAbsSum = 0;
  int32_t peak = 0;
  for (size_t i = 0; i < snapshot.samplesRead; ++i) {
    const int32_t centered = static_cast<int32_t>(samples[i]) - mean;
    const int32_t magnitude = abs32(centered);
    centeredAbsSum += magnitude;
    if (magnitude > peak) {
      peak = magnitude;
    }
  }

  snapshot.mean = mean;
  snapshot.minSample = minSample;
  snapshot.maxSample = maxSample;
  snapshot.peak = peak;
  snapshot.averageAbs =
      static_cast<int32_t>(centeredAbsSum / static_cast<int64_t>(snapshot.samplesRead));
  return snapshot;
}

bool AudioInput::readPcm16(int16_t* buffer, size_t maxSamples, size_t& samplesRead) {
  samplesRead = 0;
  if (!ready_ || buffer == nullptr || maxSamples == 0) {
    return false;
  }

  int16_t samples[kSampleCount] = {};
  size_t bytesRead = 0;
  const size_t samplesToRead = min(maxSamples, kSampleCount);
  const esp_err_t err =
      i2s_read(kMicPort, samples, samplesToRead * sizeof(samples[0]), &bytesRead, 0);
  if (err != ESP_OK || bytesRead == 0) {
    return false;
  }

  samplesRead = bytesRead / sizeof(samples[0]);
  for (size_t i = 0; i < samplesRead; ++i) {
    buffer[i] = sampleToPcm16(samples[i]);
  }
  return samplesRead > 0;
}

bool AudioInput::ready() const {
  return ready_;
}

}  // namespace tongdou
