#pragma once

#include "diagnostic/HardwareSelfTestService.h"
#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/DisplayBus.h"
#include "hardware/FaceDisplay.h"
#include "hardware/LedPixel.h"
#include "hardware/MotorDriver.h"
#include "hardware/RadarSensor.h"

namespace tongdou {

class HardwareManager {
 public:
  void begin();
  void update();

  BatteryMonitor& battery();
  FaceDisplay& faceDisplay();
  LedPixel& led();
  MotorDriver& motors();
  RadarSensor& radar();
  AudioInput& audioInput();
  AudioOutput& audioOutput();
  HardwareSelfTestService& selfTest();

 private:
  BatteryMonitor battery_;
  DisplayBus displayBus_;
  FaceDisplay faceDisplay_;
  LedPixel led_;
  MotorDriver motors_;
  RadarSensor radar_;
  AudioInput audioInput_;
  AudioOutput audioOutput_;
  HardwareSelfTestService selfTest_{battery_, audioInput_, audioOutput_, led_, motors_,
                                    radar_, faceDisplay_};
};

}  // namespace tongdou
