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

bool FaceDisplay::ready() const {
  return available_;
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

void FaceDisplay::drawChar5x7(int16_t x, int16_t y, char value, uint8_t scale) {
  uint8_t columns[5] = {};
  switch (value) {
    case 'B':
      columns[0] = 0x7F; columns[1] = 0x49; columns[2] = 0x49; columns[3] = 0x49; columns[4] = 0x36;
      break;
    case 'C':
      columns[0] = 0x3E; columns[1] = 0x41; columns[2] = 0x41; columns[3] = 0x41; columns[4] = 0x22;
      break;
    case 'D':
      columns[0] = 0x7F; columns[1] = 0x41; columns[2] = 0x41; columns[3] = 0x22; columns[4] = 0x1C;
      break;
    case 'E':
      columns[0] = 0x7F; columns[1] = 0x49; columns[2] = 0x49; columns[3] = 0x49; columns[4] = 0x41;
      break;
    case 'F':
      columns[0] = 0x7F; columns[1] = 0x09; columns[2] = 0x09; columns[3] = 0x09; columns[4] = 0x01;
      break;
    case 'O':
      columns[0] = 0x3E; columns[1] = 0x41; columns[2] = 0x41; columns[3] = 0x41; columns[4] = 0x3E;
      break;
    case 'S':
      columns[0] = 0x26; columns[1] = 0x49; columns[2] = 0x49; columns[3] = 0x49; columns[4] = 0x32;
      break;
    case 'T':
      columns[0] = 0x01; columns[1] = 0x01; columns[2] = 0x7F; columns[3] = 0x01; columns[4] = 0x01;
      break;
    case '1':
      columns[0] = 0x00; columns[1] = 0x42; columns[2] = 0x7F; columns[3] = 0x40; columns[4] = 0x00;
      break;
    case '2':
      columns[0] = 0x62; columns[1] = 0x51; columns[2] = 0x49; columns[3] = 0x49; columns[4] = 0x46;
      break;
    case '3':
      columns[0] = 0x22; columns[1] = 0x41; columns[2] = 0x49; columns[3] = 0x49; columns[4] = 0x36;
      break;
    case '4':
      columns[0] = 0x18; columns[1] = 0x14; columns[2] = 0x12; columns[3] = 0x7F; columns[4] = 0x10;
      break;
    case '5':
      columns[0] = 0x27; columns[1] = 0x45; columns[2] = 0x45; columns[3] = 0x45; columns[4] = 0x39;
      break;
    case '.':
      columns[0] = 0x00; columns[1] = 0x60; columns[2] = 0x60; columns[3] = 0x00; columns[4] = 0x00;
      break;
    case ':':
      columns[0] = 0x00; columns[1] = 0x36; columns[2] = 0x36; columns[3] = 0x00; columns[4] = 0x00;
      break;
    case ' ':
    default:
      break;
  }

  const uint8_t drawScale = scale == 0 ? 1 : scale;
  for (uint8_t col = 0; col < 5; ++col) {
    for (uint8_t row = 0; row < 7; ++row) {
      if ((columns[col] & (1U << row)) == 0) {
        continue;
      }
      fillRect(x + col * drawScale, y + row * drawScale, drawScale, drawScale);
    }
  }
}

void FaceDisplay::drawText5x7(int16_t x, int16_t y, const char* text, uint8_t scale) {
  if (text == nullptr) {
    return;
  }

  const uint8_t drawScale = scale == 0 ? 1 : scale;
  int16_t cursor = x;
  while (*text != '\0') {
    drawChar5x7(cursor, y, *text, drawScale);
    cursor += static_cast<int16_t>(6 * drawScale);
    ++text;
  }
}

void FaceDisplay::drawCoffeeDebt(uint8_t cups) {
  drawText5x7(35, 5, "COFFEE", 1);
  drawText5x7(20, 22, "DEBT:", 1);
  drawChar5x7(84, 20, cups == 2 ? '2' : '3', 3);
}

void FaceDisplay::drawDebtScratch(uint8_t scratchWidth) {
  drawCoffeeDebt(3);
  fillRect(77, 31, scratchWidth, 5);
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
    case FaceExpression::AccountantPause:
      line(30, 33, 56, 31);
      line(72, 31, 98, 33);
      break;
    case FaceExpression::AccountantRoundEyes:
      rect(30, 18, 28, 30);
      rect(70, 18, 28, 30);
      fillRect(41, 30, 7, 9);
      fillRect(81, 30, 7, 9);
      line(59, 54, 69, 54);
      break;
    case FaceExpression::AccountantSuspiciousSquint:
      line(29, 29, 57, 36);
      line(71, 36, 99, 29);
      fillRect(35, 35, 12, 4);
      fillRect(82, 35, 12, 4);
      break;
    case FaceExpression::AccountantCalculating:
      rect(33, 27, 22, 16);
      rect(73, 27, 22, 16);
      fillRect(42, 37, 7, 5);
      fillRect(82, 37, 7, 5);
      line(50, 51, 78, 51);
      break;
    case FaceExpression::AccountantLedgerSearchHigh:
      rect(30, 18, 28, 30);
      rect(70, 18, 28, 30);
      fillRect(40, 23, 8, 7);
      fillRect(80, 23, 8, 7);
      line(36, 53, 92, 53);
      break;
    case FaceExpression::AccountantLedgerSearchLow:
      rect(30, 18, 28, 30);
      rect(70, 18, 28, 30);
      fillRect(40, 37, 8, 7);
      fillRect(80, 37, 8, 7);
      line(36, 53, 92, 53);
      break;
    case FaceExpression::AccountantDebt3:
      drawCoffeeDebt(3);
      break;
    case FaceExpression::AccountantDebtScratchStart:
      drawDebtScratch(16);
      break;
    case FaceExpression::AccountantDebtScratchMid:
      drawDebtScratch(34);
      break;
    case FaceExpression::AccountantDebtScratchEnd:
      drawDebtScratch(52);
      break;
    case FaceExpression::AccountantDebt2:
      drawCoffeeDebt(2);
      break;
    case FaceExpression::AccountantDebtPi:
      drawText5x7(43, 4, "COFFEES", 1);
      drawText5x7(28, 24, "3.1415", 2);
      break;
    case FaceExpression::AccountantSmugSquint:
      line(31, 30, 57, 35);
      line(71, 35, 97, 30);
      fillRect(38, 36, 14, 4);
      fillRect(78, 36, 14, 4);
      line(55, 49, 64, 53);
      line(64, 53, 76, 47);
      break;
  }
}

}  // namespace tongdou
