#pragma once

#include "motion/MotionTypes.h"

namespace tongdou {

class MotorDriver {
 public:
  void begin();
  void drive(WheelDrive left, WheelDrive right);
  void drive(WheelDrive left, WheelDrive right, uint8_t duty);
  void drive(WheelDrive left, WheelDrive right, uint8_t leftDuty, uint8_t rightDuty);
  void stop();

 private:
  void wake();
  void sleep();
  void driveLeft(WheelDrive drive, uint8_t duty);
  void driveRight(WheelDrive drive, uint8_t duty);

  bool awake_ = false;
};

}  // namespace tongdou
