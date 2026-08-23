#pragma once

#include <Arduino.h>

#include "audio/AudioCuePlayer.h"
#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/FaceDisplay.h"
#include "hardware/ImuSensor.h"
#include "hardware/LedPixel.h"
#include "hardware/LogoTouchInput.h"
#include "hardware/MotorDriver.h"
#include "hardware/RadarSensor.h"
#include "motion/GyroMotionController.h"

namespace tongdou {

struct HardwareDiagnosticStatus {
  bool faceReady = false;
  bool imuReady = false;
  bool logoTouchReady = false;
  bool ledReady = false;
  bool audioInputReady = false;
  bool audioOutputReady = false;
  bool motorFaultActive = false;
  bool motorAvailable = true;
  bool motorFaultSimulated = false;
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
                          AudioOutput& audioOutput, AudioCuePlayer& audioCuePlayer,
                          LedPixel& led,
                          MotorDriver& motors, RadarSensor& radar, FaceDisplay& faceDisplay,
                          ImuSensor& imu, LogoTouchInput& logoTouch);

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
  void printI2cScan(Print& out) const;
  void printImu(Print& out);
  void printLogoTouch(Print& out) const;
  void printRadar(Print& out) const;
  void printRadarTarget(Print& out);
  void printRadarSamples(Print& out);
  void printRadarConfig(Print& out);
  void printRadarResolution(Print& out);
  void applyRadarNearConfig(Print& out);
  void applyRadarResolution20cm(Print& out);
  void startRadarCalibration(Print& out);
  void printMic(Print& out);
  void printMotorFault(Print& out) const;
  void setMotorFaultSimulation(bool active, Print& out);
  void testSpeaker(Print& out);
  void testAudioCue(const String& command, Print& out);
  void printAudioCueResult(const AudioCuePlaybackResult& result, Print& out) const;
  void testLed(const RgbColor& color, Print& out);
  void testMotor(WheelDrive left, WheelDrive right, Print& out);
  void stopMotorPulse();

  BatteryMonitor& battery_;
  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  AudioCuePlayer& audioCuePlayer_;
  LedPixel& led_;
  MotorDriver& motors_;
  RadarSensor& radar_;
  FaceDisplay& faceDisplay_;
  ImuSensor& imu_;
  LogoTouchInput& logoTouch_;
  GyroMotionController gyroMotion_;
  MotionFrame diagnosticMotionFrame_;
  unsigned long motorStopAtMs_ = 0;
};

}  // namespace tongdou
