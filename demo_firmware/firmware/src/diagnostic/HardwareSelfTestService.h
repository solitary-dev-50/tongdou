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
                          MotorDriver& motors, RadarSensor& radar, FaceDisplay& faceDisplay,
                          ImuSensor& imu);

  void begin();
  void update();
  HardwareDiagnosticStatus status() const;
  BatterySnapshot batterySnapshot() const;
  bool motorLeftInverted() const;
  bool motorRightInverted() const;
  uint8_t motorLeftDefaultDuty() const;
  uint8_t motorRightDefaultDuty() const;
  uint8_t audioVolumePercent() const;
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
  void printRadarConfig(Print& out);
  void printRadarResolution(Print& out);
  void applyRadarNearConfig(Print& out);
  void applyRadarResolution20cm(Print& out);
  void startRadarCalibration(Print& out);
  void printMic(Print& out);
  void printI2cScan(Print& out);
  void printImu(Print& out);
  void printImuRawTest(Print& out);
  void printImuRegisterDump(Print& out) const;
  void testSpeaker(Print& out);
  void testLed(const RgbColor& color, Print& out);
  void testMotor(WheelDrive left, WheelDrive right, Print& out);
  void startManualMotor(WheelDrive direction, const String& command, Print& out);
  void startAutoMotorCalibration(WheelDrive direction, const String& command, Print& out);
  void printAutoMotorCalibration(Print& out) const;
  void printAudioVolume(Print& out) const;
  void updateAutoMotorCalibration();
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
  unsigned long motorStopAtMs_ = 0;
  unsigned long manualMotorTimeoutMs_ = 0;
  unsigned long autoMotorNextMs_ = 0;
  unsigned long autoMotorControlNextMs_ = 0;
  unsigned long autoMotorLastSampleMs_ = 0;
  unsigned long autoMotorPhaseStartedMs_ = 0;
  unsigned long motorCalibrationNextMs_ = 0;
  uint8_t autoMotorPhase_ = 0;
  WheelDrive autoMotorDirection_ = WheelDrive::Stop;
  uint8_t autoMotorLeftPwm_ = 187;
  uint8_t autoMotorRightPwm_ = 170;
  uint8_t autoMotorNewLeftPwm_ = 187;
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
};

}  // namespace tongdou
