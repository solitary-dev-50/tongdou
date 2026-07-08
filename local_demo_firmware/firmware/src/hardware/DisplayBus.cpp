#include "hardware/DisplayBus.h"

#include <Arduino.h>
#include <Wire.h>

#include "tongdou/Pins.h"

namespace tongdou {

void DisplayBus::begin() {
  Wire.begin(static_cast<int>(pins::I2C_SDA), static_cast<int>(pins::I2C_SCL));
}

}  // namespace tongdou
