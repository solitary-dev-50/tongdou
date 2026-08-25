#pragma once

#include <Arduino.h>

#include "light/LightTypes.h"

namespace tongdou {

class LedPixel {
 public:
  void begin();
  void show(const RgbColor& color);
  void off();
  bool ready() const;
  uint32_t revision() const;

 private:
  bool ready_ = false;
  uint32_t revision_ = 0;
};

}  // namespace tongdou
