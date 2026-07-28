#pragma once

#include <stdint.h>

namespace tongdou {

enum class FaceExpression : uint8_t {
  Blank,
  Sleep,
  HalfOpen,
  Awake,
  Blink,
  Smile,
  Serious,
  RollEyesLeft,
  RollEyesRight,
  Wronged,
  Squint,
  Innocent,
  Confused,
  Angry,
  Surprised,
  Shy,
  Fierce,
  Proud,
  Nervous,
};

struct FaceFrame {
  FaceExpression expression = FaceExpression::Blank;
  uint16_t durationMs = 0;
};

}  // namespace tongdou
