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
  fillRect(leftX, y, w, h);
  fillRect(rightX, y, w, h);
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
    case FaceExpression::Smile:
      fillRect(34, 24, 18, 14);
      fillRect(76, 24, 18, 14);
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
  }
}

}  // namespace tongdou
