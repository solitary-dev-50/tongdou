#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "face/FaceExpression.h"
#include "hardware/BatteryMonitor.h"

namespace tongdou {

class FaceDisplay {
 public:
  explicit FaceDisplay(BatteryMonitor& battery) : battery_(battery) {}
  void begin();
  void show(FaceExpression expression);
  void updateStatusBar();
  void showActivationCode(const String& code);
  bool showBitmap128x48(const uint8_t* bitmap, size_t length);
  bool ready() const;
  uint32_t revision() const;

 private:
  static constexpr uint8_t kAddress = 0x3C;
  static constexpr uint8_t kWidth = 128;
  static constexpr uint8_t kHeight = 64;
  static constexpr uint16_t kBufferSize = (kWidth * kHeight) / 8;

  void command(uint8_t value);
  void flush();
  void clear();
  void pixel(int16_t x, int16_t y, bool on = true);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool on = true);
  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                     bool on = true);
  void rect(int16_t x, int16_t y, int16_t w, int16_t h, bool on = true);
  void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on = true);
  void drawEyePair(int16_t leftX, int16_t rightX, int16_t y, int16_t w, int16_t h);
  void drawDigit(char digit, int16_t x, int16_t y, uint8_t scale);
  void drawExpression(FaceExpression expression);
  void drawBatteryStatus();
  void drawBatteryIcon(uint8_t percent, bool charging, bool available);

  bool available_ = false;
  bool statusBarEnabled_ = false;
  uint32_t lastStatusBarAtMs_ = 0;
  uint16_t lastStatusVoltageMv_ = 0;
  uint8_t lastStatusPercent_ = 255;
  bool lastStatusCharging_ = false;
  bool lastStatusAvailable_ = false;
  BatteryMonitor& battery_;
  uint8_t buffer_[kBufferSize] = {};
  uint32_t revision_ = 0;
};

}  // namespace tongdou
