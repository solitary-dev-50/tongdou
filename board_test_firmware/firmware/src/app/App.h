#pragma once

#include <Arduino.h>

#include "hardware/HardwareManager.h"
#include "motion/GyroReturnWiggleController.h"
#include "web/WebConfigServer.h"

namespace tongdou {

class App {
 public:
  void begin();
  void update();

 private:
  void handleLogoTouchEvents();
  void updateLogoWiggleState();
  void handleSerialDiagnostics();
  void handleStartupReport();
  void printStartupReport(const char* title);

  HardwareManager hardware_;
  GyroReturnWiggleController logoWiggle_{hardware_.motors(), hardware_.imu()};
  WebConfigServer webConfigServer_{hardware_.selfTest()};
  unsigned long bootMs_ = 0;
  uint8_t startupReportCount_ = 0;
  bool logoWiggleWasRunning_ = false;
  bool logoSleepMode_ = false;
};

}  // namespace tongdou
