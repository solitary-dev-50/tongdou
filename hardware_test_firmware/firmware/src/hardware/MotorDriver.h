#pragma once

#include "motion/MotionTypes.h"

namespace tongdou {

struct MotorPinDiagnostic {
  const char* name = "";
  uint8_t gpio = 0;
  uint8_t pwmChannel = 0;
  uint8_t duty = 0;
  int logicLevel = 0;
};

struct MotorDriverDiagnostic {
  bool awake = false;
  WheelDrive leftCommand = WheelDrive::Stop;
  WheelDrive rightCommand = WheelDrive::Stop;
  WheelDrive leftApplied = WheelDrive::Stop;
  WheelDrive rightApplied = WheelDrive::Stop;
  uint8_t leftDuty = 0;
  uint8_t rightDuty = 0;
  MotorPinDiagnostic ain1;
  MotorPinDiagnostic ain2;
  MotorPinDiagnostic bin1;
  MotorPinDiagnostic bin2;
};

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
  MotorDriverDiagnostic diagnostic() const;
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
  bool leftInverted_ = true;
  bool rightInverted_ = true;
  uint8_t leftDefaultDuty_ = 170;
  uint8_t rightDefaultDuty_ = 170;
  WheelDrive leftCommand_ = WheelDrive::Stop;
  WheelDrive rightCommand_ = WheelDrive::Stop;
  WheelDrive leftApplied_ = WheelDrive::Stop;
  WheelDrive rightApplied_ = WheelDrive::Stop;
  uint8_t leftDuty_ = 0;
  uint8_t rightDuty_ = 0;
  uint8_t ain1Duty_ = 0;
  uint8_t ain2Duty_ = 0;
  uint8_t bin1Duty_ = 0;
  uint8_t bin2Duty_ = 0;
};

}  // namespace tongdou
