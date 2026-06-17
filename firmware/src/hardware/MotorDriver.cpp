#include "hardware/MotorDriver.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {

void MotorDriver::begin() {
  pinMode(pins::MOTOR_AIN1, OUTPUT);
  pinMode(pins::MOTOR_AIN2, OUTPUT);
  pinMode(pins::MOTOR_BIN1, OUTPUT);
  pinMode(pins::MOTOR_BIN2, OUTPUT);
  stop();
}

void MotorDriver::stop() {
  digitalWrite(pins::MOTOR_AIN1, LOW);
  digitalWrite(pins::MOTOR_AIN2, LOW);
  digitalWrite(pins::MOTOR_BIN1, LOW);
  digitalWrite(pins::MOTOR_BIN2, LOW);
}

}  // namespace tongdou
