#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tongdou {

enum class WheelDrive : uint8_t {
  Stop,
  Forward,
  Reverse,
  Brake,
};

struct MotionFrame {
  WheelDrive left = WheelDrive::Stop;
  WheelDrive right = WheelDrive::Stop;
  uint16_t durationMs = 0;
};

struct MotionSequence {
  const MotionFrame* frames = nullptr;
  size_t count = 0;
};

}  // namespace tongdou
