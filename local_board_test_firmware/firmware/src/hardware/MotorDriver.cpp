#include "hardware/MotorDriver.h"

#include <Arduino.h>
#include <Preferences.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr uint8_t kMotorAin1Channel = 0;
constexpr uint8_t kMotorAin2Channel = 1;
constexpr uint8_t kMotorBin1Channel = 2;
constexpr uint8_t kMotorBin2Channel = 3;
constexpr uint32_t kMotorPwmHz = 20000;
constexpr uint8_t kMotorPwmBits = 8;
constexpr uint8_t kDriveDuty = 200;
constexpr uint8_t kBrakeDuty = 255;
constexpr const char* kMotorPrefsNamespace = "td_motor";
constexpr const char* kLeftInvertKey = "left_inv";
constexpr const char* kRightInvertKey = "right_inv";
constexpr const char* kLeftDutyKey = "left_pwm";
constexpr const char* kRightDutyKey = "right_pwm";

void prepareStoppedOutput(gpio_num_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

void preparePwmOutput(gpio_num_t pin, uint8_t channel) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
  ledcSetup(channel, kMotorPwmHz, kMotorPwmBits);
  ledcAttachPin(pin, channel);
  ledcWrite(channel, 0);
}

uint8_t clampDuty(uint8_t duty) {
  return duty == 0 ? kDriveDuty : duty;
}

uint8_t scaledDuty(uint8_t duty, uint8_t calibratedDuty) {
  const uint16_t value =
      (static_cast<uint16_t>(duty) * static_cast<uint16_t>(calibratedDuty) +
       (kDriveDuty / 2)) /
      kDriveDuty;
  return static_cast<uint8_t>(constrain(value, 1, 255));
}

}  // namespace

void MotorDriver::begin() {
  Preferences prefs;
  if (prefs.begin(kMotorPrefsNamespace, true)) {
    leftInverted_ = prefs.getBool(kLeftInvertKey, leftInverted_);
    rightInverted_ = prefs.getBool(kRightInvertKey, rightInverted_);
    leftDefaultDuty_ =
        static_cast<uint8_t>(prefs.getUChar(kLeftDutyKey, leftDefaultDuty_));
    rightDefaultDuty_ =
        static_cast<uint8_t>(prefs.getUChar(kRightDutyKey, rightDefaultDuty_));
    prefs.end();
  }

  prepareStoppedOutput(pins::MOTOR_SLEEP);
  preparePwmOutput(pins::MOTOR_AIN1, kMotorAin1Channel);
  preparePwmOutput(pins::MOTOR_AIN2, kMotorAin2Channel);
  preparePwmOutput(pins::MOTOR_BIN1, kMotorBin1Channel);
  preparePwmOutput(pins::MOTOR_BIN2, kMotorBin2Channel);
  stop();
}

void MotorDriver::drive(WheelDrive left, WheelDrive right) {
  drive(left, right, kDriveDuty);
}

void MotorDriver::drive(WheelDrive left, WheelDrive right, uint8_t duty) {
  drive(left, right, duty, 0, 0);
}

void MotorDriver::drive(WheelDrive left, WheelDrive right, uint8_t duty,
                        uint8_t leftDuty, uint8_t rightDuty) {
  if (left == WheelDrive::Stop && right == WheelDrive::Stop) {
    stop();
    return;
  }

  wake();
  const uint8_t baseDuty = duty == 0 ? kDriveDuty : duty;
  driveLeft(left, leftDuty == 0 ? scaledDuty(baseDuty, leftDefaultDuty_) : leftDuty);
  driveRight(right, rightDuty == 0 ? scaledDuty(baseDuty, rightDefaultDuty_) : rightDuty);
}

void MotorDriver::stop() {
  driveLeft(WheelDrive::Stop, 0);
  driveRight(WheelDrive::Stop, 0);
  sleep();
}

void MotorDriver::wake() {
  if (awake_) {
    return;
  }

  digitalWrite(pins::MOTOR_SLEEP, HIGH);
  delayMicroseconds(1000);
  awake_ = true;
}

void MotorDriver::sleep() {
  digitalWrite(pins::MOTOR_SLEEP, LOW);
  awake_ = false;
}

bool MotorDriver::leftInverted() const {
  return leftInverted_;
}

bool MotorDriver::rightInverted() const {
  return rightInverted_;
}

uint8_t MotorDriver::leftDefaultDuty() const {
  return leftDefaultDuty_;
}

uint8_t MotorDriver::rightDefaultDuty() const {
  return rightDefaultDuty_;
}

void MotorDriver::setCalibration(bool leftInverted, bool rightInverted,
                                 uint8_t leftDuty, uint8_t rightDuty) {
  leftInverted_ = leftInverted;
  rightInverted_ = rightInverted;
  leftDefaultDuty_ = clampDuty(leftDuty);
  rightDefaultDuty_ = clampDuty(rightDuty);
  stop();
}

void MotorDriver::saveCalibration(bool leftInverted, bool rightInverted,
                                  uint8_t leftDuty, uint8_t rightDuty) {
  setCalibration(leftInverted, rightInverted, leftDuty, rightDuty);

  Preferences prefs;
  if (!prefs.begin(kMotorPrefsNamespace, false)) {
    Serial.println(F("motor calibration save failed: preferences open failed"));
    return;
  }

  prefs.putBool(kLeftInvertKey, leftInverted_);
  prefs.putBool(kRightInvertKey, rightInverted_);
  prefs.putUChar(kLeftDutyKey, leftDefaultDuty_);
  prefs.putUChar(kRightDutyKey, rightDefaultDuty_);
  prefs.end();
  Serial.print(F("motor calibration saved left_inverted="));
  Serial.print(leftInverted_ ? F("1") : F("0"));
  Serial.print(F(" right_inverted="));
  Serial.print(rightInverted_ ? F("1") : F("0"));
  Serial.print(F(" left_pwm="));
  Serial.print(leftDefaultDuty_);
  Serial.print(F(" right_pwm="));
  Serial.println(rightDefaultDuty_);
}

WheelDrive MotorDriver::applyLeftDirection(WheelDrive drive) const {
  if (!leftInverted_) {
    return drive;
  }
  if (drive == WheelDrive::Forward) {
    return WheelDrive::Reverse;
  }
  if (drive == WheelDrive::Reverse) {
    return WheelDrive::Forward;
  }
  return drive;
}

WheelDrive MotorDriver::applyRightDirection(WheelDrive drive) const {
  if (!rightInverted_) {
    return drive;
  }
  if (drive == WheelDrive::Forward) {
    return WheelDrive::Reverse;
  }
  if (drive == WheelDrive::Reverse) {
    return WheelDrive::Forward;
  }
  return drive;
}

void MotorDriver::driveLeft(WheelDrive drive, uint8_t duty) {
  drive = applyLeftDirection(drive);
  const uint8_t activeDuty = duty == 0 ? kDriveDuty : duty;
  switch (drive) {
    case WheelDrive::Forward:
      ledcWrite(kMotorAin2Channel, 0);
      ledcWrite(kMotorAin1Channel, activeDuty);
      break;
    case WheelDrive::Reverse:
      ledcWrite(kMotorAin1Channel, 0);
      ledcWrite(kMotorAin2Channel, activeDuty);
      break;
    case WheelDrive::Brake:
      ledcWrite(kMotorAin1Channel, duty == 0 ? kBrakeDuty : duty);
      ledcWrite(kMotorAin2Channel, duty == 0 ? kBrakeDuty : duty);
      break;
    case WheelDrive::Stop:
    default:
      ledcWrite(kMotorAin1Channel, 0);
      ledcWrite(kMotorAin2Channel, 0);
      break;
  }
}

void MotorDriver::driveRight(WheelDrive drive, uint8_t duty) {
  drive = applyRightDirection(drive);
  const uint8_t activeDuty = duty == 0 ? kDriveDuty : duty;
  switch (drive) {
    case WheelDrive::Forward:
      ledcWrite(kMotorBin1Channel, 0);
      ledcWrite(kMotorBin2Channel, activeDuty);
      break;
    case WheelDrive::Reverse:
      ledcWrite(kMotorBin2Channel, 0);
      ledcWrite(kMotorBin1Channel, activeDuty);
      break;
    case WheelDrive::Brake:
      ledcWrite(kMotorBin1Channel, duty == 0 ? kBrakeDuty : duty);
      ledcWrite(kMotorBin2Channel, duty == 0 ? kBrakeDuty : duty);
      break;
    case WheelDrive::Stop:
    default:
      ledcWrite(kMotorBin1Channel, 0);
      ledcWrite(kMotorBin2Channel, 0);
      break;
  }
}

}  // namespace tongdou
