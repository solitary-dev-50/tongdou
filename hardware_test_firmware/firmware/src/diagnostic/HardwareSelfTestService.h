#pragma once

#include <Arduino.h>

#include "diagnostic/AudioLoopbackTest.h"
#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/FaceDisplay.h"
#include "hardware/ImuSensor.h"
#include "hardware/LedPixel.h"
#include "hardware/LogoTouchInput.h"
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
                          ImuSensor& imu, LogoTouchInput& logoTouch);

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
  bool shippingDischargeActive() const;
  bool shippingDischargeDone() const;
  bool shippingDischargeFailed() const;
  uint8_t shippingDischargeTargetPercent() const;
  const char* shippingDischargeMessage() const;
  bool audioRecordingAvailable() const;
  uint32_t audioRecordingSampleRateHz() const;
  const int16_t* audioRecordingSamples() const;
  size_t audioRecordingSampleCount() const;
  size_t audioRecordingByteCount() const;
  void saveMotorCalibration(bool leftInverted, bool rightInverted,
                            uint8_t leftDuty, uint8_t rightDuty);
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
  void setRadarEngineeringMode(bool enabled, Print& out);
  void applyRadarNearConfig(Print& out);
  void applyRadarDeskMode(Print& out);
  void applyRadarResolution20cm(Print& out);
  void startRadarCalibration(Print& out);
  void printRadarCalibrationStatus(Print& out);
  void restoreRadarFactoryDefaults(Print& out);
  void startRadarBridge(Print& out);
  void printMic(Print& out);
  void printMicMode(Print& out) const;
  void setMicMode(const String& command, Print& out);
  void startAudioLoopback(Print& out);
  void printAudioLoopbackStatus(Print& out) const;
  void stopAudioLoopback(Print& out);
  void printI2cScan(Print& out);
  void printImu(Print& out);
  void printImuRawTest(Print& out);
  void printImuRegisterDump(Print& out) const;
  void printLogoTouch(Print& out) const;
  void watchLogoTouch(Print& out);
  void testSpeaker(Print& out);
  void testLed(const RgbColor& color, Print& out);
  void testMotor(WheelDrive left, WheelDrive right, Print& out);
  void runMotorDriverDiagnostic(Print& out, bool testA, bool testB);
  void printMotorDriverDiagnostic(const __FlashStringHelper* step, Print& out) const;
  void startManualMotor(WheelDrive direction, const String& command, Print& out);
  void startAutoMotorCalibration(WheelDrive direction, const String& command, Print& out);
  void printAutoMotorCalibration(Print& out) const;
  void printAudioVolume(Print& out) const;
  void startShippingDischarge(Print& out);
  void stopShippingDischarge(Print& out);
  void printShippingDischargeStatus(Print& out) const;
  void pulseLedDataPin(Print& out);
  void updateAutoMotorCalibration();
  void updateRadarGuidedTest();
  void updateBatteryDisplay();
  void updateShippingDischarge();
  void finishShippingDischarge(bool failed, const char* message, bool alarm);
  void finishAutoMotorCalibration();
  void cancelAutoMotorCalibration();
  void sampleAutoMotorGyro(bool probePhase);
  void resetAutoMotorSamples();
  void startMotorCalibration(WheelDrive direction, const String& command, Print& out);
  void stopMotorPulse();

  BatteryMonitor& battery_;
  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  LedPixel& led_;
  MotorDriver& motors_;
  RadarSensor& radar_;
  FaceDisplay& faceDisplay_;
  ImuSensor& imu_;
  LogoTouchInput& logoTouch_;
  AudioLoopbackTest audioLoopback_;
  unsigned long nextBatteryDisplayMs_ = 0;
  unsigned long shippingDischargeStartedMs_ = 0;
  unsigned long shippingDischargePhaseStartedMs_ = 0;
  bool shippingDischargeActive_ = false;
  bool shippingDischargeDone_ = false;
  bool shippingDischargeFailed_ = false;
  bool shippingDischargeMotorOn_ = false;
  char shippingDischargeMessage_[96] = "idle";
  unsigned long motorStopAtMs_ = 0;
  unsigned long manualMotorTimeoutMs_ = 0;
  unsigned long autoMotorNextMs_ = 0;
  unsigned long autoMotorControlNextMs_ = 0;
  unsigned long autoMotorLastSampleMs_ = 0;
  unsigned long autoMotorPhaseStartedMs_ = 0;
  unsigned long motorCalibrationNextMs_ = 0;
  uint8_t autoMotorPhase_ = 0;
  WheelDrive autoMotorDirection_ = WheelDrive::Stop;
  uint8_t autoMotorLeftPwm_ = 170;
  uint8_t autoMotorRightPwm_ = 170;
  uint8_t autoMotorNewLeftPwm_ = 170;
  uint8_t autoMotorNewRightPwm_ = 170;
  uint16_t autoMotorProbeSamples_ = 0;
  uint16_t autoMotorStraightSamples_ = 0;
  int32_t autoMotorBiasX_ = 0;
  int32_t autoMotorBiasY_ = 0;
  int32_t autoMotorBiasZ_ = 0;
  int64_t autoMotorBiasSumX_ = 0;
  int64_t autoMotorBiasSumY_ = 0;
  int64_t autoMotorBiasSumZ_ = 0;
  uint16_t autoMotorBiasSamples_ = 0;
  int64_t autoMotorProbeAngleX_ = 0;
  int64_t autoMotorProbeAngleY_ = 0;
  int64_t autoMotorProbeAngleZ_ = 0;
  int64_t autoMotorStraightAngleX_ = 0;
  int64_t autoMotorStraightAngleY_ = 0;
  int64_t autoMotorStraightAngleZ_ = 0;
  int16_t autoMotorCorrection_ = 0;
  int16_t autoMotorLiveCorrection_ = 0;
  int16_t autoMotorBiasTerm_ = 0;
  int32_t autoMotorLastYawRate_ = 0;
  int32_t autoMotorLastAlignedRate_ = 0;
  int32_t autoMotorLastAlignedAngle_ = 0;
  int16_t autoMotorLastRateCorrection_ = 0;
  int16_t autoMotorLastHeadingCorrection_ = 0;
  int8_t autoMotorAxis_ = -1;
  bool autoMotorDone_ = false;
  bool autoMotorFailed_ = false;
  char autoMotorMessage_[96] = "idle";
  uint8_t motorCalibrationPhase_ = 0;
  WheelDrive motorCalibrationDirection_ = WheelDrive::Stop;
  uint8_t motorCalibrationLeftStartPwm_ = 210;
  uint8_t motorCalibrationRightStartPwm_ = 210;
  uint8_t motorCalibrationLeftHoldPwm_ = 150;
  uint8_t motorCalibrationRightHoldPwm_ = 150;
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
