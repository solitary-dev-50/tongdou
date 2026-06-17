#pragma once

#include <Arduino.h>

#include "hardware/BatteryMonitor.h"
#include "hardware/DisplayBus.h"
#include "hardware/LedPixel.h"
#include "hardware/MotorDriver.h"
#include "hardware/RadarSensor.h"
#include "hardware/VoiceUart.h"

namespace tongdou {

class App {
 public:
  void begin();
  void update();

 private:
  void logHeartbeat();

  BatteryMonitor battery_;
  DisplayBus displayBus_;
  LedPixel led_;
  MotorDriver motors_;
  RadarSensor radar_;
  VoiceUart voice_;
  unsigned long lastHeartbeatMs_ = 0;
};

}  // namespace tongdou
