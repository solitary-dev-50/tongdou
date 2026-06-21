#include "hardware/RadarSensor.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {

void RadarSensor::begin() {
  pinMode(pins::RADAR_OUT, INPUT);
  pinMode(pins::RADAR_RX_FROM_MODULE_TX, INPUT);
  // RADAR_TX_TO_MODULE_RX is owned by the future UART setup.
}

RadarSnapshot RadarSensor::read() const {
  RadarSnapshot snapshot;
  snapshot.occupied = digitalRead(pins::RADAR_OUT) == HIGH;
  snapshot.rxLevelHigh = digitalRead(pins::RADAR_RX_FROM_MODULE_TX) == HIGH;
  return snapshot;
}

}  // namespace tongdou
