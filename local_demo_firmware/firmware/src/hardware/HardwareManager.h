#pragma once

#include <Arduino.h>

#include "diagnostic/HardwareSelfTestService.h"
#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/DisplayBus.h"
#include "hardware/FaceDisplay.h"
#include "hardware/ImuSensor.h"
#include "hardware/LedPixel.h"
#include "hardware/LogoTouchInput.h"
#include "hardware/MotorDriver.h"
#include "hardware/RadarSensor.h"

namespace tongdou {

class HardwareManager {
 public:
  void begin();
  void update();
  void printStartupReport(Print& out) const;

  BatteryMonitor& battery();
  FaceDisplay& faceDisplay();
  LedPixel& led();
  MotorDriver& motors();
  RadarSensor& radar();
  AudioInput& audioInput();
  AudioOutput& audioOutput();
  ImuSensor& imu();
  LogoTouchInput& logoTouch();
  HardwareSelfTestService& selfTest();

 private:
  BatteryMonitor battery_;
  DisplayBus displayBus_;
  FaceDisplay faceDisplay_;
  ImuSensor imu_;
  LedPixel led_;
  LogoTouchInput logoTouch_;
  MotorDriver motors_;
  RadarSensor radar_;
  AudioInput audioInput_;
  AudioOutput audioOutput_;
  BatterySnapshot startupBattery_;
  HardwareSelfTestService selfTest_{battery_, audioInput_, audioOutput_, led_, motors_,
                                    radar_, faceDisplay_, imu_};
};

}  // namespace tongdou
