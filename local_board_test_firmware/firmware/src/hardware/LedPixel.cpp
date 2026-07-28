#include "hardware/LedPixel.h"

#include <driver/rmt.h>
#include <freertos/FreeRTOS.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr rmt_channel_t kRmtChannel = RMT_CHANNEL_0;
constexpr uint8_t kRmtClockDiv = 2;
constexpr uint16_t kTick0High = 14;
constexpr uint16_t kTick0Low = 32;
constexpr uint16_t kTick1High = 28;
constexpr uint16_t kTick1Low = 24;
constexpr uint8_t kColorByteCount = 3;
constexpr uint8_t kBitsPerByte = 8;
constexpr uint8_t kWs2812BitCount = kColorByteCount * kBitsPerByte;

uint8_t scaleChannelToFullBrightness(uint8_t value, uint8_t maxChannel) {
  if (value == 0 || maxChannel == 0) {
    return 0;
  }
  return static_cast<uint8_t>((static_cast<uint16_t>(value) * 255U) / maxChannel);
}

rmt_item32_t makeBit(bool enabled) {
  rmt_item32_t item = {};
  item.level0 = 1;
  item.duration0 = enabled ? kTick1High : kTick0High;
  item.level1 = 0;
  item.duration1 = enabled ? kTick1Low : kTick0Low;
  return item;
}

}  // namespace

void LedPixel::begin() {
  rmt_config_t config = {};
  config.rmt_mode = RMT_MODE_TX;
  config.channel = kRmtChannel;
  config.gpio_num = pins::WS2812_DATA;
  config.mem_block_num = 1;
  config.clk_div = kRmtClockDiv;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

  const esp_err_t configResult = rmt_config(&config);
  const esp_err_t installResult = rmt_driver_install(kRmtChannel, 0, 0);
  ready_ = (configResult == ESP_OK) && (installResult == ESP_OK);
  if (!ready_) {
    pinMode(pins::WS2812_DATA, OUTPUT);
  }
  off();
}

void LedPixel::show(const RgbColor& color) {
  if (!ready_) {
    return;
  }

  const uint8_t maxChannel = max(color.red, max(color.green, color.blue));
  const RgbColor fullBrightness = {
      scaleChannelToFullBrightness(color.red, maxChannel),
      scaleChannelToFullBrightness(color.green, maxChannel),
      scaleChannelToFullBrightness(color.blue, maxChannel),
  };
  const uint8_t bytes[kColorByteCount] = {fullBrightness.green, fullBrightness.red,
                                          fullBrightness.blue};
  rmt_item32_t items[kWs2812BitCount] = {};
  size_t itemIndex = 0;

  for (uint8_t byte : bytes) {
    for (int bit = kBitsPerByte - 1; bit >= 0; --bit) {
      items[itemIndex++] = makeBit((byte & (1 << bit)) != 0);
    }
  }

  rmt_write_items(kRmtChannel, items, kWs2812BitCount, true);
  rmt_wait_tx_done(kRmtChannel, pdMS_TO_TICKS(10));
  delayMicroseconds(80);
}

void LedPixel::off() {
  if (!ready_) {
    digitalWrite(pins::WS2812_DATA, LOW);
    return;
  }

  show({0, 0, 0});
}

bool LedPixel::ready() const {
  return ready_;
}

}  // namespace tongdou
