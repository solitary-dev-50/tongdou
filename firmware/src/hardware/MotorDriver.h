#pragma once

#include "motion/MotionTypes.h"

namespace tongdou {

class MotorDriver {
 public:
  void begin();
  void drive(WheelDrive left, WheelDrive right);
  void stop();

 private:
  void wake();
  void sleep();
  void driveLeft(WheelDrive drive);
  void driveRight(WheelDrive drive);

  bool awake_ = false;
};

}  // namespace tongdou
