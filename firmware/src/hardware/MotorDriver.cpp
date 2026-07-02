#include "hardware/MotorDriver.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

void prepareStoppedOutput(gpio_num_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

}  // namespace

void MotorDriver::begin() {
  prepareStoppedOutput(pins::MOTOR_AIN1);
  prepareStoppedOutput(pins::MOTOR_AIN2);
  prepareStoppedOutput(pins::MOTOR_BIN1);
  prepareStoppedOutput(pins::MOTOR_BIN2);
  stop();
}

void MotorDriver::drive(WheelDrive left, WheelDrive right) {
  driveLeft(left);
  driveRight(right);
}

void MotorDriver::stop() {
  drive(WheelDrive::Stop, WheelDrive::Stop);
}

void MotorDriver::driveLeft(WheelDrive drive) {
  switch (drive) {
    case WheelDrive::Forward:
      digitalWrite(pins::MOTOR_AIN1, HIGH);
      digitalWrite(pins::MOTOR_AIN2, LOW);
      break;
    case WheelDrive::Reverse:
      digitalWrite(pins::MOTOR_AIN1, LOW);
      digitalWrite(pins::MOTOR_AIN2, HIGH);
      break;
    case WheelDrive::Stop:
    default:
      digitalWrite(pins::MOTOR_AIN1, LOW);
      digitalWrite(pins::MOTOR_AIN2, LOW);
      break;
  }
}

void MotorDriver::driveRight(WheelDrive drive) {
  switch (drive) {
    case WheelDrive::Forward:
      digitalWrite(pins::MOTOR_BIN1, HIGH);
      digitalWrite(pins::MOTOR_BIN2, LOW);
      break;
    case WheelDrive::Reverse:
      digitalWrite(pins::MOTOR_BIN1, LOW);
      digitalWrite(pins::MOTOR_BIN2, HIGH);
      break;
    case WheelDrive::Stop:
    default:
      digitalWrite(pins::MOTOR_BIN1, LOW);
      digitalWrite(pins::MOTOR_BIN2, LOW);
      break;
  }
}

}  // namespace tongdou
