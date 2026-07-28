#pragma once

#include <Arduino.h>

namespace tongdou {

struct BatterySnapshot {
  bool usbPresent = false;
  bool charging = false;
  bool standby = false;
  int rawAdc = 0;
};

class BatteryMonitor {
 public:
  void begin();
  BatterySnapshot read() const;
};

}  // namespace tongdou
