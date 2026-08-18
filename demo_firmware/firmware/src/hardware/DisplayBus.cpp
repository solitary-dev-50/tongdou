#include "hardware/DisplayBus.h"

#include <Arduino.h>
#include <Wire.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr uint32_t kI2cClockHz = 50000;

}  // namespace

void DisplayBus::begin() {
  Wire.begin(static_cast<int>(pins::I2C_SDA), static_cast<int>(pins::I2C_SCL));
  Wire.setClock(kI2cClockHz);
}

}  // namespace tongdou
