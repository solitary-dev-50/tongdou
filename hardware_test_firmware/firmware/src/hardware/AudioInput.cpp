#include "hardware/AudioInput.h"

#include <driver/i2s.h>
#include <esp_err.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kSampleCount = 128;

enum class MicSampleFormat : uint8_t {
  Pcm16,
  Pcm32High16,
  Pcm32Low16,
};

struct MicCaptureMode {
  const char* name;
  i2s_channel_fmt_t channel;
  bool clockOnWs;
  bool setDownsample;
  i2s_pdm_dsr_t downsample;
  i2s_bits_per_sample_t bitsPerSample;
  MicSampleFormat sampleFormat;
};

constexpr MicCaptureMode kMicModes[] = {
    {"right_ws_default", I2S_CHANNEL_FMT_ONLY_RIGHT, true, false, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"left_ws_default", I2S_CHANNEL_FMT_ONLY_LEFT, true, false, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"right_bck_default", I2S_CHANNEL_FMT_ONLY_RIGHT, false, false, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"left_bck_default", I2S_CHANNEL_FMT_ONLY_LEFT, false, false, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"right_ws_dsr8", I2S_CHANNEL_FMT_ONLY_RIGHT, true, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"left_ws_dsr8", I2S_CHANNEL_FMT_ONLY_LEFT, true, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"right_ws_dsr16", I2S_CHANNEL_FMT_ONLY_RIGHT, true, true, I2S_PDM_DSR_16S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"left_ws_dsr16", I2S_CHANNEL_FMT_ONLY_LEFT, true, true, I2S_PDM_DSR_16S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"right_bck_dsr8", I2S_CHANNEL_FMT_ONLY_RIGHT, false, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"left_bck_dsr8", I2S_CHANNEL_FMT_ONLY_LEFT, false, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_16BIT, MicSampleFormat::Pcm16},
    {"right_ws_32_high", I2S_CHANNEL_FMT_ONLY_RIGHT, true, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32High16},
    {"left_ws_32_high", I2S_CHANNEL_FMT_ONLY_LEFT, true, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32High16},
    {"right_ws_32_low", I2S_CHANNEL_FMT_ONLY_RIGHT, true, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32Low16},
    {"left_ws_32_low", I2S_CHANNEL_FMT_ONLY_LEFT, true, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32Low16},
    {"right_bck_32_high", I2S_CHANNEL_FMT_ONLY_RIGHT, false, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32High16},
    {"left_bck_32_high", I2S_CHANNEL_FMT_ONLY_LEFT, false, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32High16},
    {"right_bck_32_low", I2S_CHANNEL_FMT_ONLY_RIGHT, false, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32Low16},
    {"left_bck_32_low", I2S_CHANNEL_FMT_ONLY_LEFT, false, true, I2S_PDM_DSR_8S,
     I2S_BITS_PER_SAMPLE_32BIT, MicSampleFormat::Pcm32Low16},
};

int32_t abs32(int32_t value) {
  return value < 0 ? -value : value;
}

int16_t sampleToPcm16(int32_t raw) {
  return static_cast<int16_t>(raw);
}

int16_t sampleToPcm16(int32_t raw, MicSampleFormat format) {
  switch (format) {
    case MicSampleFormat::Pcm16:
      return static_cast<int16_t>(raw);
    case MicSampleFormat::Pcm32High16:
      return static_cast<int16_t>(raw >> 16);
    case MicSampleFormat::Pcm32Low16:
      return static_cast<int16_t>(raw & 0xffff);
  }
  return static_cast<int16_t>(raw);
}

const char* sampleFormatName(MicSampleFormat format) {
  switch (format) {
    case MicSampleFormat::Pcm16:
      return "pcm16";
    case MicSampleFormat::Pcm32High16:
      return "pcm32_high16";
    case MicSampleFormat::Pcm32Low16:
      return "pcm32_low16";
  }
  return "unknown";
}

}  // namespace

void AudioInput::begin() {
  installMode(modeIndex_, Serial);
}

bool AudioInput::installMode(uint8_t modeIndex, Print& out) {
  if (modeIndex >= modeCount()) {
    out.print(F("mic mode invalid index="));
    out.println(modeIndex);
    return false;
  }

  const MicCaptureMode& mode = kMicModes[modeIndex];

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  config.sample_rate = kSampleRate;
  config.bits_per_sample = mode.bitsPerSample;
  config.channel_format = mode.channel;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = mode.clockOnWs ? I2S_PIN_NO_CHANGE : tongdou::pins::PDM_MIC_CLK;
  pins.ws_io_num = mode.clockOnWs ? tongdou::pins::PDM_MIC_CLK : I2S_PIN_NO_CHANGE;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = tongdou::pins::PDM_MIC_DATA;

  esp_err_t err = i2s_driver_install(kMicPort, &config, 0, nullptr);
  if (err == ESP_OK) {
    installed_ = true;
    err = i2s_set_pin(kMicPort, &pins);
  }
  if (err == ESP_OK && mode.setDownsample) {
    err = i2s_set_pdm_rx_down_sample(kMicPort, mode.downsample);
  }
  if (err == ESP_OK) {
    i2s_zero_dma_buffer(kMicPort);
    ready_ = true;
    modeIndex_ = modeIndex;
    printMode(out);
    return true;
  }

  ready_ = false;
  out.print(F("mic input failed mode="));
  out.print(modeIndex);
  out.print(F(" err="));
  out.println(static_cast<int>(err));
  uninstall();
  return false;
}

void AudioInput::uninstall() {
  ready_ = false;
  if (installed_) {
    i2s_driver_uninstall(kMicPort);
    installed_ = false;
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
  int32_t wideSamples[kSampleCount] = {};
  size_t bytesRead = 0;
  esp_err_t err = ESP_OK;
  if (kMicModes[modeIndex_].bitsPerSample == I2S_BITS_PER_SAMPLE_32BIT) {
    err = i2s_read(kMicPort, wideSamples, sizeof(wideSamples), &bytesRead,
                   pdMS_TO_TICKS(20));
  } else {
    err = i2s_read(kMicPort, samples, sizeof(samples), &bytesRead,
                   pdMS_TO_TICKS(20));
  }
  if (err != ESP_OK || bytesRead == 0) {
    return snapshot;
  }

  const MicCaptureMode& mode = kMicModes[modeIndex_];
  snapshot.samplesRead =
      mode.bitsPerSample == I2S_BITS_PER_SAMPLE_32BIT
          ? bytesRead / sizeof(wideSamples[0])
          : bytesRead / sizeof(samples[0]);
  snapshot.hasSamples = snapshot.samplesRead > 0;
  for (size_t i = 0; i < snapshot.samplesRead && i < kSampleCount; ++i) {
    if (mode.bitsPerSample == I2S_BITS_PER_SAMPLE_32BIT) {
      samples[i] = sampleToPcm16(wideSamples[i], mode.sampleFormat);
    }
  }

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
  int32_t wideSamples[kSampleCount] = {};
  size_t bytesRead = 0;
  const size_t samplesToRead = min(maxSamples, kSampleCount);
  const MicCaptureMode& mode = kMicModes[modeIndex_];
  esp_err_t err = ESP_OK;
  if (mode.bitsPerSample == I2S_BITS_PER_SAMPLE_32BIT) {
    err = i2s_read(kMicPort, wideSamples, samplesToRead * sizeof(wideSamples[0]),
                   &bytesRead, pdMS_TO_TICKS(20));
  } else {
    err = i2s_read(kMicPort, samples, samplesToRead * sizeof(samples[0]),
                   &bytesRead, pdMS_TO_TICKS(20));
  }
  if (err != ESP_OK || bytesRead == 0) {
    return false;
  }

  samplesRead = mode.bitsPerSample == I2S_BITS_PER_SAMPLE_32BIT
                    ? bytesRead / sizeof(wideSamples[0])
                    : bytesRead / sizeof(samples[0]);
  for (size_t i = 0; i < samplesRead; ++i) {
    buffer[i] = mode.bitsPerSample == I2S_BITS_PER_SAMPLE_32BIT
                    ? sampleToPcm16(wideSamples[i], mode.sampleFormat)
                    : sampleToPcm16(samples[i]);
  }
  return samplesRead > 0;
}

bool AudioInput::ready() const {
  return ready_;
}

uint8_t AudioInput::modeIndex() const {
  return modeIndex_;
}

uint8_t AudioInput::modeCount() const {
  return static_cast<uint8_t>(sizeof(kMicModes) / sizeof(kMicModes[0]));
}

bool AudioInput::setMode(uint8_t modeIndex, Print& out) {
  if (modeIndex >= modeCount()) {
    out.print(F("mic mode invalid index="));
    out.println(modeIndex);
    return false;
  }

  uninstall();
  delay(20);
  return installMode(modeIndex, out);
}

void AudioInput::printMode(Print& out) const {
  const MicCaptureMode& mode = kMicModes[modeIndex_];
  out.println(F("mic mode:"));
  out.print(F("  index="));
  out.println(modeIndex_);
  out.print(F("  name="));
  out.println(mode.name);
  out.print(F("  channel="));
  out.println(mode.channel == I2S_CHANNEL_FMT_ONLY_RIGHT ? F("right") : F("left"));
  out.print(F("  clock_pin_role="));
  out.println(mode.clockOnWs ? F("ws") : F("bck"));
  out.print(F("  pdm_clk_io="));
  out.println(static_cast<int>(tongdou::pins::PDM_MIC_CLK));
  out.print(F("  pdm_data_io="));
  out.println(static_cast<int>(tongdou::pins::PDM_MIC_DATA));
  out.print(F("  bits_per_sample="));
  out.println(static_cast<int>(mode.bitsPerSample));
  out.print(F("  sample_format="));
  out.println(sampleFormatName(mode.sampleFormat));
  out.print(F("  downsample="));
  if (!mode.setDownsample) {
    out.println(F("driver_default"));
  } else {
    out.println(mode.downsample == I2S_PDM_DSR_16S ? F("16s") : F("8s"));
  }
  out.print(F("  ready="));
  out.println(ready_ ? 1 : 0);
}

}  // namespace tongdou
