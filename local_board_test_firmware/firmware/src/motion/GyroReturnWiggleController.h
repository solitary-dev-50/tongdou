#pragma once

#include <Arduino.h>

#include "hardware/ImuSensor.h"
#include "hardware/MotorDriver.h"

namespace tongdou {

class GyroReturnWiggleController {
 public:
  GyroReturnWiggleController(MotorDriver& motors, ImuSensor& imu);

  void start();
  void update();
  void stop();
  bool running() const;

 private:
  enum class Phase : uint8_t {
    Idle,
    Bias,
    WiggleLeft1,
    Pause1,
    WiggleRight1,
    Pause2,
    WiggleLeft2,
    Pause3,
    WiggleRight2,
    Settle,
    ReturnProbe,
    ReturnDrive,
  };

  void startPhase(Phase phase, unsigned long durationMs);
  void startDrivePhase(Phase phase, unsigned long durationMs, WheelDrive left,
                       WheelDrive right, uint8_t duty);
  void updateGyro(unsigned long now);
  void finishBias();
  void enterReturn(unsigned long now);
  void updateReturn(unsigned long now);
  void driveReturn(int8_t direction);
  void finish(const __FlashStringHelper* reason);
  int32_t absAngle() const;

  MotorDriver& motors_;
  ImuSensor& imu_;
  Phase phase_ = Phase::Idle;
  unsigned long phaseStartedMs_ = 0;
  unsigned long phaseDurationMs_ = 0;
  unsigned long lastGyroMs_ = 0;
  unsigned long returnStartedMs_ = 0;
  unsigned long returnProbeStartedMs_ = 0;
  int64_t biasSumZ_ = 0;
  uint16_t biasSamples_ = 0;
  int32_t biasZ_ = 0;
  int32_t yawAngleMdeg_ = 0;
  int32_t returnProbeStartAbsMdeg_ = 0;
  int8_t returnDirection_ = 0;
  bool gyroOk_ = false;
};

}  // namespace tongdou
