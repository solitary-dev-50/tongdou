#pragma once

#include <Arduino.h>

namespace tongdou {

struct BatterySnapshot {
  bool usbPresent = false;
  bool charging = false;
  bool standby = false;
  int rawAdc = 0;
  uint16_t voltageMv = 0;
  uint8_t percent = 0;
};

class BatteryMonitor {
 public:
  void begin();
  BatterySnapshot read() const;
};

}  // namespace tongdou
