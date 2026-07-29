#pragma once

#include <Arduino.h>

#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/FaceDisplay.h"
#include "hardware/ImuSensor.h"
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
  bool webAvailable = true;
  bool visualFeedbackAvailable = false;
  bool soundAvailable = false;
  bool micInputAvailable = false;
};

class HardwareSelfTestService {
 public:
  HardwareSelfTestService(BatteryMonitor& battery, AudioInput& audioInput,
                          AudioOutput& audioOutput, LedPixel& led,
                          MotorDriver& motors, RadarSensor& radar, FaceDisplay& faceDisplay,
                          ImuSensor& imu);

  void begin();
  void update();
  HardwareDiagnosticStatus status() const;
  BatterySnapshot batterySnapshot() const;
  RadarSnapshot radarSnapshot() const;
  RadarTargetSnapshot radarTargetSnapshot();
  bool motorLeftInverted() const;
  bool motorRightInverted() const;
  uint8_t motorLeftDefaultDuty() const;
  uint8_t motorRightDefaultDuty() const;
  uint8_t audioVolumePercent() const;
  void saveAudioVolumePercent(uint8_t percent);
  void printHelp(Print& out) const;
  bool handleCommand(const String& command, Print& out);

 private:
  void printReport(Print& out);
  void printBattery(Print& out) const;
  void printStatus(Print& out) const;
  void printRadar(Print& out) const;
  void printRadarTarget(Print& out);
  void printRadarSamples(Print& out);
  void printRadarParserSelfTest(Print& out) const;
  void runRadarGuidedTest(Print& out);
  void startRadarGuidedTest(Print& out);
  void printRadarGuidedStatus(Print& out) const;
  void printRadarConfig(Print& out);
  void printRadarResolution(Print& out);
  void printRadarSensitivity(Print& out);
  void applyRadarNearConfig(Print& out);
  void applyRadarDeskMode(Print& out);
  void applyRadarResolution20cm(Print& out);
  void startRadarCalibration(Print& out);
  void printRadarCalibrationStatus(Print& out);
  void startRadarBridge(Print& out);
  void printMic(Print& out);
  void printI2cScan(Print& out);
  void printImu(Print& out);
  void printImuRawTest(Print& out);
  void printImuRegisterDump(Print& out) const;
  void testSpeaker(Print& out);
  void testLed(const RgbColor& color, Print& out);
  void testMotor(WheelDrive left, WheelDrive right, Print& out);
  void startManualMotor(WheelDrive direction, const String& command, Print& out);
  void printAudioVolume(Print& out) const;
  void updateRadarGuidedTest();
  void stopMotorPulse();

  BatteryMonitor& battery_;
  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  LedPixel& led_;
  MotorDriver& motors_;
  RadarSensor& radar_;
  FaceDisplay& faceDisplay_;
  ImuSensor& imu_;
  unsigned long motorStopAtMs_ = 0;
  unsigned long manualMotorTimeoutMs_ = 0;
  bool radarGuidedActive_ = false;
  bool radarGuidedDone_ = false;
  uint8_t radarGuidedPhase_ = 0;
  unsigned long radarGuidedPhaseStartMs_ = 0;
  unsigned long radarGuidedNextSampleMs_ = 0;
  uint32_t radarGuidedLastSequence_ = 0;
  uint16_t radarGuidedSamples_[5] = {};
  uint16_t radarGuidedReceivedFrames_[5] = {};
  uint16_t radarGuidedTargetFrames_[5] = {};
  uint16_t radarGuidedStateTargetFrames_[5] = {};
  uint16_t radarGuidedEnergyTargetFrames_[5] = {};
  uint16_t radarGuidedOutHighFrames_[5] = {};
  uint16_t radarGuidedFirstTargetMs_[5] = {};
  uint16_t radarGuidedMinTargetDistanceCm_[5] = {};
  uint8_t radarGuidedMaxMovingEnergy_[5] = {};
  uint8_t radarGuidedMaxStaticEnergy_[5] = {};
  uint8_t radarGuidedMaxConfidence_[5] = {};
  uint8_t radarGuidedLastState_[5] = {};
};

}  // namespace tongdou
