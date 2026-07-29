#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tongdou {

struct RgbColor {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

struct LightFrame {
  RgbColor color;
  uint16_t durationMs = 0;
};

struct LightSequence {
  const LightFrame* frames = nullptr;
  size_t count = 0;
};

}  // namespace tongdou
