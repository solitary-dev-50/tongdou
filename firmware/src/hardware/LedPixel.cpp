#include "hardware/LedPixel.h"

#include <driver/rmt.h>
#include <freertos/FreeRTOS.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr rmt_channel_t kRmtChannel = RMT_CHANNEL_0;
constexpr uint8_t kRmtClockDiv = 2;
constexpr uint16_t kTick0High = 12;
constexpr uint16_t kTick0Low = 36;
constexpr uint16_t kTick1High = 24;
constexpr uint16_t kTick1Low = 24;
constexpr uint16_t kResetDelayUs = 300;
constexpr uint8_t kColorByteCount = 3;
constexpr uint8_t kBitsPerByte = 8;
constexpr uint8_t kLedBitCount = kColorByteCount * kBitsPerByte;

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
  config.gpio_num = pins::LED_DATA;
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
    pinMode(pins::LED_DATA, OUTPUT);
  }
  off();
}

void LedPixel::show(const RgbColor& color) {
  if (!ready_) {
    return;
  }

  const uint8_t bytes[kColorByteCount] = {color.green, color.red, color.blue};
  rmt_item32_t items[kLedBitCount] = {};
  size_t itemIndex = 0;

  for (uint8_t byte : bytes) {
    for (int bit = kBitsPerByte - 1; bit >= 0; --bit) {
      items[itemIndex++] = makeBit((byte & (1 << bit)) != 0);
    }
  }

  rmt_write_items(kRmtChannel, items, kLedBitCount, true);
  rmt_wait_tx_done(kRmtChannel, pdMS_TO_TICKS(10));
  delayMicroseconds(kResetDelayUs);
}

void LedPixel::off() {
  if (!ready_) {
    digitalWrite(pins::LED_DATA, LOW);
    return;
  }

  show({0, 0, 0});
}

bool LedPixel::ready() const {
  return ready_;
}

}  // namespace tongdou
