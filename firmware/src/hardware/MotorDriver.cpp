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
  prepareStoppedOutput(pins::MOTOR_SLEEP);
  prepareStoppedOutput(pins::MOTOR_AIN1);
  prepareStoppedOutput(pins::MOTOR_AIN2);
  prepareStoppedOutput(pins::MOTOR_BIN1);
  prepareStoppedOutput(pins::MOTOR_BIN2);
  stop();
}

void MotorDriver::drive(WheelDrive left, WheelDrive right) {
  if (left == WheelDrive::Stop && right == WheelDrive::Stop) {
    stop();
    return;
  }

  wake();
  driveLeft(left);
  driveRight(right);
}

void MotorDriver::stop() {
  driveLeft(WheelDrive::Stop);
  driveRight(WheelDrive::Stop);
  sleep();
}

void MotorDriver::wake() {
  if (awake_) {
    return;
  }

  digitalWrite(pins::MOTOR_SLEEP, HIGH);
  delay(2);
  awake_ = true;
}

void MotorDriver::sleep() {
  digitalWrite(pins::MOTOR_SLEEP, LOW);
  awake_ = false;
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
