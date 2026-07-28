#pragma once

#include "motion/MotionTypes.h"

namespace tongdou {

class MotorDriver {
 public:
  void begin();
  void drive(WheelDrive left, WheelDrive right);
  void drive(WheelDrive left, WheelDrive right, uint8_t duty);
  void drive(WheelDrive left, WheelDrive right, uint8_t duty, uint8_t leftDuty,
             uint8_t rightDuty);
  void stop();
  bool leftInverted() const;
  bool rightInverted() const;
  uint8_t leftDefaultDuty() const;
  uint8_t rightDefaultDuty() const;
  void setCalibration(bool leftInverted, bool rightInverted, uint8_t leftDuty,
                      uint8_t rightDuty);
  void saveCalibration(bool leftInverted, bool rightInverted, uint8_t leftDuty,
                       uint8_t rightDuty);

 private:
  void wake();
  void sleep();
  WheelDrive applyLeftDirection(WheelDrive drive) const;
  WheelDrive applyRightDirection(WheelDrive drive) const;
  void driveLeft(WheelDrive drive, uint8_t duty);
  void driveRight(WheelDrive drive, uint8_t duty);

  bool awake_ = false;
  bool leftInverted_ = false;
  bool rightInverted_ = true;
  uint8_t leftDefaultDuty_ = 187;
  uint8_t rightDefaultDuty_ = 170;
};

}  // namespace tongdou
