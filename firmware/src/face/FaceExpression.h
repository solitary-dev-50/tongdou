#pragma once

#include <stdint.h>

namespace tongdou {

enum class FaceExpression : uint8_t {
  Blank,
  Sleep,
  HalfOpen,
  Awake,
  Smile,
  Serious,
  RollEyesLeft,
  RollEyesRight,
  Wronged,
  Squint,
  Innocent,
  Confused,
};

struct FaceFrame {
  FaceExpression expression = FaceExpression::Blank;
  uint16_t durationMs = 0;
};

}  // namespace tongdou
