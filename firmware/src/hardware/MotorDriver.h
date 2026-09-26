#pragma once

#include "motion/MotionTypes.h"

namespace tongdou {

class MotorDriver {
 public:
  void begin();
  void drive(WheelDrive left, WheelDrive right);
  void drive(WheelDrive left, WheelDrive right, uint8_t duty);
  void drive(WheelDrive left, WheelDrive right, uint8_t leftDuty, uint8_t rightDuty);
  void drive(WheelDrive left, WheelDrive right, uint8_t duty, uint8_t leftDuty,
             uint8_t rightDuty);
  void driveAtPercent(WheelDrive left, WheelDrive right, uint8_t percent);
  void stop();
  bool leftInverted() const;
  bool rightInverted() const;
  uint8_t leftDefaultDuty() const;
  uint8_t rightDefaultDuty() const;
  void setCalibration(bool leftInverted, bool rightInverted, uint8_t leftDuty,
                      uint8_t rightDuty);
  void saveCalibration(bool leftInverted, bool rightInverted, uint8_t leftDuty,
                       uint8_t rightDuty);
  bool faultActive() const;
  bool faultPinLow() const;
  bool faultSimulated() const;
  void setFaultSimulation(bool active);
  bool available() const;

 private:
  void wake();
  void sleep();
  WheelDrive applyLeftDirection(WheelDrive drive) const;
  WheelDrive applyRightDirection(WheelDrive drive) const;
  void driveLeft(WheelDrive drive, uint8_t duty);
  void driveRight(WheelDrive drive, uint8_t duty);

  bool awake_ = false;
  bool faultSimulated_ = false;
  bool leftInverted_ = false;
  bool rightInverted_ = true;
  uint8_t leftDefaultDuty_ = 190;
  uint8_t rightDefaultDuty_ = 190;
};

}  // namespace tongdou
