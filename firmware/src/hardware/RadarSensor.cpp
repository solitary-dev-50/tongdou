#include "hardware/RadarSensor.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {

void RadarSensor::begin() {
  pinMode(pins::RADAR_OT1, INPUT);
  pinMode(pins::RADAR_RX, INPUT);
  pinMode(pins::RADAR_OT2, INPUT);
}

}  // namespace tongdou
