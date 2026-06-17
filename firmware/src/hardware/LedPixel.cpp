#include "hardware/LedPixel.h"

#include "tongdou/Pins.h"

namespace tongdou {

void LedPixel::begin() {
  pinMode(pins::WS2812_DATA, OUTPUT);
  off();
}

void LedPixel::off() {
  digitalWrite(pins::WS2812_DATA, LOW);
}

}  // namespace tongdou
