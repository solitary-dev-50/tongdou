#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tongdou {

enum class WheelDrive : uint8_t {
  Stop,
  Forward,
  Reverse,
};

struct MotionFrame {
  WheelDrive left = WheelDrive::Stop;
  WheelDrive right = WheelDrive::Stop;
  uint16_t durationMs = 0;
  uint8_t duty = 190;
};

struct MotionSequence {
  const MotionFrame* frames = nullptr;
  size_t count = 0;
  uint16_t maxReturnMs = 900;
  // 0 沿用普通动作帧力度；非零值按用户保存的左右轮力度取百分比。
  uint8_t calibratedDutyPercent = 0;
};

}  // namespace tongdou
