#include "hardware/MotorDriver.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr uint8_t kMotorAin1Channel = 0;
constexpr uint8_t kMotorAin2Channel = 1;
constexpr uint8_t kMotorBin1Channel = 2;
constexpr uint8_t kMotorBin2Channel = 3;
constexpr uint32_t kMotorPwmHz = 20000;
constexpr uint8_t kMotorPwmBits = 8;
constexpr uint8_t kDefaultDuty = 190;

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

uint8_t normalizedDuty(uint8_t duty) {
  return duty == 0 ? kDefaultDuty : duty;
}

}  // namespace

void MotorDriver::begin() {
  prepareStoppedOutput(pins::MOTOR_SLEEP);
  preparePwmOutput(pins::MOTOR_AIN1, kMotorAin1Channel);
  preparePwmOutput(pins::MOTOR_AIN2, kMotorAin2Channel);
  preparePwmOutput(pins::MOTOR_BIN1, kMotorBin1Channel);
  preparePwmOutput(pins::MOTOR_BIN2, kMotorBin2Channel);
  stop();
}

void MotorDriver::drive(WheelDrive left, WheelDrive right) {
  drive(left, right, kDefaultDuty);
}

void MotorDriver::drive(WheelDrive left, WheelDrive right, uint8_t duty) {
  const uint8_t activeDuty = normalizedDuty(duty);
  drive(left, right, activeDuty, activeDuty);
}

void MotorDriver::drive(WheelDrive left, WheelDrive right, uint8_t leftDuty,
                        uint8_t rightDuty) {
  if (left == WheelDrive::Stop && right == WheelDrive::Stop) {
    stop();
    return;
  }

  wake();
  driveLeft(left, normalizedDuty(leftDuty));
  driveRight(right, normalizedDuty(rightDuty));
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

void MotorDriver::driveLeft(WheelDrive drive, uint8_t duty) {
  switch (drive) {
    case WheelDrive::Forward:
      ledcWrite(kMotorAin2Channel, 0);
      ledcWrite(kMotorAin1Channel, duty);
      break;
    case WheelDrive::Reverse:
      ledcWrite(kMotorAin1Channel, 0);
      ledcWrite(kMotorAin2Channel, duty);
      break;
    case WheelDrive::Stop:
    default:
      ledcWrite(kMotorAin1Channel, 0);
      ledcWrite(kMotorAin2Channel, 0);
      break;
  }
}

void MotorDriver::driveRight(WheelDrive drive, uint8_t duty) {
  switch (drive) {
    case WheelDrive::Forward:
      ledcWrite(kMotorBin2Channel, 0);
      ledcWrite(kMotorBin1Channel, duty);
      break;
    case WheelDrive::Reverse:
      ledcWrite(kMotorBin1Channel, 0);
      ledcWrite(kMotorBin2Channel, duty);
      break;
    case WheelDrive::Stop:
    default:
      ledcWrite(kMotorBin1Channel, 0);
      ledcWrite(kMotorBin2Channel, 0);
      break;
  }
}

}  // namespace tongdou
