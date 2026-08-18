#pragma once

#include <stdint.h>

#include "face/FaceExpression.h"

namespace tongdou {

class FaceDisplay {
 public:
  void begin();
  void show(FaceExpression expression);
  bool ready() const;

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
  void drawChar5x7(int16_t x, int16_t y, char value, uint8_t scale = 1);
  void drawText5x7(int16_t x, int16_t y, const char* text, uint8_t scale = 1);
  void drawCoffeeDebt(uint8_t cups);
  void drawDebtScratch(uint8_t scratchWidth);
  void drawExpression(FaceExpression expression);

  bool available_ = false;
  uint8_t buffer_[kBufferSize] = {};
};

}  // namespace tongdou
