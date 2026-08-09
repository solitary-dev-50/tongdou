#include "hardware/LedPixel.h"

#include "tongdou/Pins.h"

namespace tongdou {

void LedPixel::begin() {
  pinMode(pins::LED_DATA, OUTPUT);
  ready_ = true;
  off();
}

void LedPixel::show(const RgbColor& color) {
  if (!ready_) {
    return;
  }

  neopixelWrite(pins::LED_DATA, color.red, color.green, color.blue);
  delayMicroseconds(300);
}

void LedPixel::off() {
  neopixelWrite(pins::LED_DATA, 0, 0, 0);
  digitalWrite(pins::LED_DATA, LOW);
}

bool LedPixel::ready() const {
  return ready_;
}

}  // namespace tongdou
