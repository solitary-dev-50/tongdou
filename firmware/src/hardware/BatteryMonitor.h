#pragma once

#include <Arduino.h>
#include "hardware/Bq27220.h"
#include "tongdou/BoardProfile.h"

namespace tongdou {

struct BatterySnapshot {
  bool usbPresent = false;
  bool charging = false;
  bool standby = false;
  int rawAdc = 0;
  uint16_t adcVoltageMv = 0;
  uint16_t voltageMv = 0;
  bool usingFuelGauge = false;
  Bq27220Snapshot gauge;
  const char* gaugeError = "not_fitted";
};

class BatteryMonitor {
 public:
  void begin();
  void update();
  BatterySnapshot read() const;

 private:
  void sampleAdc();
  int rawAdc_ = 0;
  uint16_t adcVoltageMv_ = 0;
  uint32_t lastAdcAtMs_ = 0;
  bool wasUsingFuelGauge_ = false;
#if TONGDOU_BOARD_REVISION == 94
  Bq27220 gauge_{Wire};
#endif
};

}  // namespace tongdou
