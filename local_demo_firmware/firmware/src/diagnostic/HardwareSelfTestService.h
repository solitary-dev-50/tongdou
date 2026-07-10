#pragma once

#include <Arduino.h>

#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/FaceDisplay.h"
#include "hardware/LedPixel.h"
#include "hardware/MotorDriver.h"
#include "hardware/RadarSensor.h"

namespace tongdou {

struct HardwareDiagnosticStatus {
  bool faceReady = false;
  bool ledReady = false;
  bool audioInputReady = false;
  bool audioOutputReady = false;
  bool degraded = false;
  bool remindersAvailable = true;
  bool webAvailable = true;
  bool visualFeedbackAvailable = false;
  bool soundAvailable = false;
  bool voiceInputAvailable = false;
};

class HardwareSelfTestService {
 public:
  HardwareSelfTestService(BatteryMonitor& battery, AudioInput& audioInput,
                          AudioOutput& audioOutput, LedPixel& led,
                          MotorDriver& motors, RadarSensor& radar, FaceDisplay& faceDisplay);

  void begin();
  void update();
  HardwareDiagnosticStatus status() const;
  BatterySnapshot batterySnapshot() const;
  void printHelp(Print& out) const;
  bool handleCommand(const String& command, Print& out);

 private:
  void printReport(Print& out);
  void printBattery(Print& out) const;
  void printStatus(Print& out) const;
  void printRadar(Print& out) const;
  void printRadarTarget(Print& out);
  void printRadarSamples(Print& out);
  void printRadarConfig(Print& out);
  void printRadarResolution(Print& out);
  void applyRadarNearConfig(Print& out);
  void applyRadarResolution20cm(Print& out);
  void startRadarCalibration(Print& out);
  void printMic(Print& out);
  void testSpeaker(Print& out);
  void testLed(const RgbColor& color, Print& out);
  void testMotor(WheelDrive left, WheelDrive right, Print& out);
  void startManualMotor(WheelDrive direction, const String& command, Print& out);
  void startMotorCalibration(WheelDrive direction, const String& command, Print& out);
  void stopMotorPulse();

  BatteryMonitor& battery_;
  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  LedPixel& led_;
  MotorDriver& motors_;
  RadarSensor& radar_;
  FaceDisplay& faceDisplay_;
  unsigned long motorStopAtMs_ = 0;
  unsigned long manualMotorTimeoutMs_ = 0;
  unsigned long motorCalibrationNextMs_ = 0;
  uint8_t motorCalibrationPhase_ = 0;
  WheelDrive motorCalibrationDirection_ = WheelDrive::Stop;
  uint8_t motorCalibrationLeftStartPwm_ = 210;
  uint8_t motorCalibrationRightStartPwm_ = 210;
  uint8_t motorCalibrationLeftHoldPwm_ = 150;
  uint8_t motorCalibrationRightHoldPwm_ = 150;
};

}  // namespace tongdou
