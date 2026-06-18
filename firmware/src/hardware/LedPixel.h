#pragma once

#include <Arduino.h>

#include "light/LightTypes.h"

namespace tongdou {

class LedPixel {
 public:
  void begin();
  void show(const RgbColor& color);
  void off();

 private:
  bool ready_ = false;
};

}  // namespace tongdou
