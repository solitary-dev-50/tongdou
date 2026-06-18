#pragma once

#include "motion/MotionTypes.h"

namespace tongdou {

class MotorDriver {
 public:
  void begin();
  void drive(WheelDrive left, WheelDrive right);
  void stop();

 private:
  void driveLeft(WheelDrive drive);
  void driveRight(WheelDrive drive);
};

}  // namespace tongdou
