#include "hardware/FaceDisplay.h"

#include <Arduino.h>
#include <Wire.h>
#include <string.h>

namespace tongdou {

void FaceDisplay::begin() {
  Wire.beginTransmission(kAddress);
  available_ = (Wire.endTransmission() == 0);
  if (!available_) {
    Serial.println("face display not found");
    return;
  }

  command(0xAE);  // display off
  command(0xD5);
  command(0x80);
  command(0xA8);
  command(0x3F);
  command(0xD3);
  command(0x00);
  command(0x40);
  command(0x8D);
  command(0x14);
  command(0x20);
  command(0x00);
  command(0xA1);
  command(0xC8);
  command(0xDA);
  command(0x12);
  command(0x81);
  command(0x8F);
  command(0xD9);
  command(0xF1);
  command(0xDB);
  command(0x40);
  command(0xA4);
  command(0xA6);
  command(0xAF);  // display on

  show(FaceExpression::Sleep);
}

void FaceDisplay::show(FaceExpression expression) {
  if (!available_) {
    return;
  }

  clear();
  drawExpression(expression);
  flush();
  ++revision_;
}

void FaceDisplay::showActivationCode(const String& code) {
  if (!available_) {
    return;
  }

  clear();
  rect(8, 8, 112, 48);
  const uint8_t scale = 3;
  const int16_t digitWidth = 5 * scale;
  const int16_t spacing = 3;
  const int16_t count = min<int16_t>(6, code.length());
  const int16_t totalWidth = count * digitWidth + max<int16_t>(0, count - 1) * spacing;
  int16_t x = static_cast<int16_t>((kWidth - totalWidth) / 2);
  const int16_t y = 23;

  for (int16_t i = 0; i < count; ++i) {
    drawDigit(code[static_cast<unsigned int>(i)], x, y, scale);
    x += digitWidth + spacing;
  }

  flush();
  ++revision_;
}

bool FaceDisplay::showBitmap128x48(const uint8_t* bitmap, size_t length) {
  constexpr size_t kBitmapSize = (kWidth * 48) / 8;
  if (!available_ || bitmap == nullptr || length != kBitmapSize) {
    return false;
  }

  clear();
  memcpy(buffer_ + kWidth, bitmap, kBitmapSize);
  flush();
  ++revision_;
  return true;
}

bool FaceDisplay::ready() const {
  return available_;
}

uint32_t FaceDisplay::revision() const {
  return revision_;
}

void FaceDisplay::command(uint8_t value) {
  Wire.beginTransmission(kAddress);
  Wire.write(0x00);
  Wire.write(value);
  Wire.endTransmission();
}

void FaceDisplay::flush() {
  for (uint8_t page = 0; page < 8; ++page) {
    command(static_cast<uint8_t>(0xB0 + page));
    command(0x00);
    command(0x10);

    for (uint8_t chunk = 0; chunk < 8; ++chunk) {
      Wire.beginTransmission(kAddress);
      Wire.write(0x40);
      const uint16_t offset = static_cast<uint16_t>(page) * kWidth + chunk * 16;
      for (uint8_t i = 0; i < 16; ++i) {
        Wire.write(buffer_[offset + i]);
      }
      Wire.endTransmission();
    }
  }
}

void FaceDisplay::clear() {
  memset(buffer_, 0, sizeof(buffer_));
}

void FaceDisplay::pixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) {
    return;
  }

  const uint16_t index = static_cast<uint16_t>(x) + (static_cast<uint16_t>(y) / 8) * kWidth;
  const uint8_t mask = static_cast<uint8_t>(1U << (y & 7));
  if (on) {
    buffer_[index] |= mask;
  } else {
    buffer_[index] &= static_cast<uint8_t>(~mask);
  }
}

void FaceDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool on) {
  for (int16_t yy = y; yy < y + h; ++yy) {
    for (int16_t xx = x; xx < x + w; ++xx) {
      pixel(xx, yy, on);
    }
  }
}

void FaceDisplay::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                                bool on) {
  if (r <= 0 || h <= r * 2 || w <= r * 2) {
    fillRect(x, y, w, h, on);
    return;
  }

  fillRect(x + r, y, w - r * 2, h, on);
  fillRect(x, y + r, w, h - r * 2, on);

  for (int16_t yy = 0; yy < r; ++yy) {
    for (int16_t xx = 0; xx < r; ++xx) {
      const int16_t dx = static_cast<int16_t>(r - 1 - xx);
      const int16_t dy = static_cast<int16_t>(r - 1 - yy);
      if (dx * dx + dy * dy <= r * r) {
        pixel(x + xx, y + yy, on);
        pixel(x + w - 1 - xx, y + yy, on);
        pixel(x + xx, y + h - 1 - yy, on);
        pixel(x + w - 1 - xx, y + h - 1 - yy, on);
      }
    }
  }
}

void FaceDisplay::rect(int16_t x, int16_t y, int16_t w, int16_t h, bool on) {
  line(x, y, x + w - 1, y, on);
  line(x, y + h - 1, x + w - 1, y + h - 1, on);
  line(x, y, x, y + h - 1, on);
  line(x + w - 1, y, x + w - 1, y + h - 1, on);
}

void FaceDisplay::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on) {
  const int16_t dx = abs(x1 - x0);
  const int16_t sx = x0 < x1 ? 1 : -1;
  const int16_t dy = -abs(y1 - y0);
  const int16_t sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;

  while (true) {
    pixel(x0, y0, on);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int16_t e2 = static_cast<int16_t>(2 * err);
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void FaceDisplay::drawEyePair(int16_t leftX, int16_t rightX, int16_t y, int16_t w,
                              int16_t h) {
  const int16_t radius = h > 8 ? 4 : 2;
  fillRoundRect(leftX, y, w, h, radius);
  fillRoundRect(rightX, y, w, h, radius);
}

void FaceDisplay::drawDigit(char digit, int16_t x, int16_t y, uint8_t scale) {
  static constexpr uint8_t kSegments[10] = {
      0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
      0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111,
  };

  if (digit < '0' || digit > '9' || scale == 0) {
    return;
  }

  const uint8_t mask = kSegments[digit - '0'];
  const int16_t t = scale;
  const int16_t w = 5 * scale;
  const int16_t h = 9 * scale;
  const int16_t mid = y + 4 * scale;

  if (mask & 0b00000001) {
    fillRect(x + t, y, w - 2 * t, t);
  }
  if (mask & 0b00000010) {
    fillRect(x + w - t, y + t, t, 3 * scale);
  }
  if (mask & 0b00000100) {
    fillRect(x + w - t, mid + t, t, 3 * scale);
  }
  if (mask & 0b00001000) {
    fillRect(x + t, y + h - t, w - 2 * t, t);
  }
  if (mask & 0b00010000) {
    fillRect(x, mid + t, t, 3 * scale);
  }
  if (mask & 0b00100000) {
    fillRect(x, y + t, t, 3 * scale);
  }
  if (mask & 0b01000000) {
    fillRect(x + t, mid, w - 2 * t, t);
  }
}

void FaceDisplay::drawExpression(FaceExpression expression) {
  switch (expression) {
    case FaceExpression::Blank:
      break;
    case FaceExpression::Sleep:
      line(32, 34, 54, 34);
      line(74, 34, 96, 34);
      break;
    case FaceExpression::HalfOpen:
      drawEyePair(34, 76, 30, 18, 6);
      break;
    case FaceExpression::Awake:
      drawEyePair(34, 76, 24, 18, 18);
      break;
    case FaceExpression::Blink:
      drawEyePair(34, 76, 32, 18, 3);
      break;
    case FaceExpression::Smile:
      drawEyePair(34, 76, 24, 18, 14);
      line(38, 44, 48, 49);
      line(80, 49, 90, 44);
      break;
    case FaceExpression::Serious:
      fillRect(32, 29, 24, 8);
      fillRect(72, 29, 24, 8);
      break;
    case FaceExpression::RollEyesLeft:
      rect(31, 23, 24, 20);
      rect(73, 23, 24, 20);
      fillRect(34, 27, 8, 8);
      fillRect(76, 27, 8, 8);
      break;
    case FaceExpression::RollEyesRight:
      rect(31, 23, 24, 20);
      rect(73, 23, 24, 20);
      fillRect(44, 27, 8, 8);
      fillRect(86, 27, 8, 8);
      break;
    case FaceExpression::Wronged:
      line(31, 25, 55, 36);
      line(73, 36, 97, 25);
      fillRect(40, 38, 8, 8);
      fillRect(82, 38, 8, 8);
      break;
    case FaceExpression::Squint:
      line(30, 29, 56, 35);
      line(72, 35, 98, 29);
      line(30, 35, 56, 29);
      line(72, 29, 98, 35);
      break;
    case FaceExpression::Innocent:
      rect(31, 20, 26, 26);
      rect(71, 20, 26, 26);
      fillRect(40, 29, 8, 8);
      fillRect(80, 29, 8, 8);
      pixel(50, 24);
      pixel(90, 24);
      break;
    case FaceExpression::Confused:
      rect(32, 24, 22, 18);
      line(75, 24, 97, 42);
      line(75, 42, 97, 24);
      fillRect(39, 31, 7, 7);
      break;
    case FaceExpression::Angry:
      line(30, 22, 56, 30);
      line(72, 30, 98, 22);
      fillRoundRect(34, 31, 20, 10, 3);
      fillRoundRect(74, 31, 20, 10, 3);
      break;
    case FaceExpression::Surprised:
      rect(32, 20, 24, 28);
      rect(72, 20, 24, 28);
      fillRect(41, 31, 6, 8);
      fillRect(81, 31, 6, 8);
      line(58, 52, 70, 52);
      break;
    case FaceExpression::Shy:
      drawEyePair(36, 78, 28, 14, 10);
      line(25, 45, 31, 42);
      line(97, 42, 103, 45);
      pixel(29, 47);
      pixel(99, 47);
      break;
    case FaceExpression::Fierce:
      line(28, 22, 58, 34);
      line(70, 34, 100, 22);
      fillRoundRect(34, 34, 22, 8, 2);
      fillRoundRect(72, 34, 22, 8, 2);
      break;
    case FaceExpression::Proud:
      line(31, 31, 55, 27);
      line(73, 27, 97, 31);
      fillRoundRect(36, 33, 16, 8, 2);
      fillRoundRect(78, 33, 16, 8, 2);
      line(54, 48, 64, 52);
      line(64, 52, 74, 48);
      break;
    case FaceExpression::Nervous:
      rect(32, 24, 22, 18);
      rect(74, 26, 20, 16);
      fillRect(43, 31, 6, 6);
      fillRect(77, 32, 6, 6);
      line(99, 22, 104, 27);
      line(104, 27, 101, 33);
      break;
  }
}

}  // namespace tongdou
