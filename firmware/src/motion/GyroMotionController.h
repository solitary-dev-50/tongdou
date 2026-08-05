#pragma once

#include <Arduino.h>

#include "hardware/ImuSensor.h"
#include "hardware/MotorDriver.h"
#include "motion/MotionTypes.h"

namespace tongdou {

class GyroMotionController {
 public:
  GyroMotionController(MotorDriver& motors, ImuSensor& imu);

  void begin();
  void start(MotionSequence sequence);
  void update();
  void stop();
  bool running() const;

 private:
  enum class Phase : uint8_t {
    Idle,
    Bias,
    Frame,
    Return,
  };

  void updateGyro(unsigned long now);
  void finishBias(unsigned long now);
  void startFrame(unsigned long now);
  void driveCurrentFrame();
  void startReturn(unsigned long now);
  void driveReturn();
  void finish(const __FlashStringHelper* reason);
  bool currentFrameNeedsYawHold() const;
  int32_t absYaw() const;

  MotorDriver& motors_;
  ImuSensor& imu_;
  MotionSequence sequence_;
  Phase phase_ = Phase::Idle;
  size_t frameIndex_ = 0;
  unsigned long phaseStartedMs_ = 0;
  unsigned long frameDurationMs_ = 0;
  unsigned long lastGyroMs_ = 0;
  unsigned long returnStartedMs_ = 0;
  int64_t biasSumZ_ = 0;
  uint16_t biasSamples_ = 0;
  int32_t biasZ_ = 0;
  int32_t yawMdeg_ = 0;
  int32_t latestYawRateMdegPerSec_ = 0;
  uint8_t currentDuty_ = 0;
  bool gyroOk_ = false;
};

}  // namespace tongdou
