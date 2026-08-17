#include "diagnostic/HardwareSelfTestService.h"

#include <Wire.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr unsigned long kMotorPulseMs = 120;
constexpr unsigned long kManualMotorTimeoutMs = 450;
constexpr unsigned long kMotorCalibrationStartMs = 90;
constexpr unsigned long kMotorCalibrationHoldMs = 820;
constexpr unsigned long kMotorCalibrationBrakeMs = 90;
constexpr unsigned long kMotorAutoBiasMs = 320;
constexpr unsigned long kMotorAutoSoftStartMs = 240;
constexpr unsigned long kMotorAutoStraightMs = 1800;
constexpr unsigned long kMotorAutoBrakeMs = 120;
constexpr unsigned long kMotorAutoSampleMs = 20;
constexpr unsigned long kMotorAutoControlMs = 30;
constexpr unsigned long kImuRawBiasMs = 800;
constexpr unsigned long kImuRawMotionMs = 3200;
constexpr unsigned long kImuRawSampleMs = 20;
constexpr unsigned long kRadarGuidedBlockingPhaseMs = 5000;
constexpr unsigned long kRadarGuidedEmptyPhaseMs = 7000;
constexpr unsigned long kRadarGuidedTargetPhaseMs = 4000;
constexpr unsigned long kRadarGuidedSamplePauseMs = 120;
constexpr unsigned long kBatteryDisplayIntervalMs = 3000;
constexpr unsigned long kShippingDischargeOnMs = 8000;
constexpr unsigned long kShippingDischargeRestMs = 2000;
constexpr unsigned long kShippingDischargeMaxMs = 45UL * 60UL * 1000UL;
constexpr uint8_t kShippingDischargeTargetPercent = 30;
constexpr uint8_t kShippingDischargePwm = 210;
constexpr uint16_t kShippingDischargeAlarmHz = 1760;
constexpr uint16_t kShippingDischargeAlarmMs = 180;
constexpr uint16_t kSpeakerTestHz = 880;
constexpr uint16_t kSpeakerTestMs = 120;
constexpr uint8_t kMotorDriverDiagnosticPwm = 200;
constexpr unsigned long kMotorDriverDiagnosticStepMs = 2000;
constexpr uint8_t kMotorCalibrationBrakePwm = 160;
constexpr uint8_t kMotorAutoBasePwm = 185;
constexpr uint8_t kMotorAutoSoftStartPwmDrop = 5;
constexpr uint8_t kMotorAutoMinPwm = 160;
constexpr uint8_t kMotorAutoMaxPwm = 220;
constexpr int16_t kMotorAutoMaxCorrection = 14;
constexpr int16_t kMotorAutoMaxLiveCorrection = 10;
constexpr int32_t kMotorAutoRateDeadband = 300;
constexpr int32_t kMotorAutoRateCorrectionDivisor = 1200;
constexpr int32_t kMotorAutoHeadingDeadband = 800;
constexpr int32_t kMotorAutoHeadingCorrectionDivisor = 1200;
constexpr int16_t kMotorAutoMaxHeadingCorrection = 8;
constexpr int8_t kMotorAutoForwardProbeSign = -1;
constexpr uint8_t kQmiStatus0 = 0x2E;
constexpr uint8_t kQmiStatus1 = 0x2F;
constexpr uint8_t kQmiTimestampL = 0x30;
constexpr uint8_t kQmiTempL = 0x33;
constexpr uint8_t kQmiAccelXoutL = 0x35;
constexpr uint8_t kQmiGyroXoutL = 0x3B;
constexpr uint8_t kQmiTempAccelGyroReadLength = 14;
constexpr int32_t kImuGyroLsbPerDps = 64;

struct ImuDebugFrame {
  bool ready = false;
  uint8_t status0 = 0;
  uint8_t status1 = 0;
  uint32_t timestamp = 0;
  int16_t tempRaw = 0;
  int16_t accelRaw[3] = {};
  int16_t gyroRaw[3] = {};
};

struct RadarGuidedPhaseStats {
  uint16_t samples = 0;
  uint16_t receivedFrames = 0;
  uint16_t targetFrames = 0;
  uint16_t stateTargetFrames = 0;
  uint16_t energyTargetFrames = 0;
  uint16_t occupiedFrames = 0;
  uint16_t firstTargetMs = 0;
  uint16_t minTargetDistanceCm = 0;
  uint8_t maxMovingEnergy = 0;
  uint8_t maxStaticEnergy = 0;
  uint8_t maxConfidence = 0;
  uint8_t lastState = 0;
};

void printBool(Print& out, const __FlashStringHelper* label, bool value) {
  out.print(label);
  out.println(value ? F("1") : F("0"));
}

void printHexByte(Print& out, uint8_t value) {
  if (value < 0x10) {
    out.print('0');
  }
  out.print(value, HEX);
}

int16_t readInt16Le(const uint8_t* buffer) {
  return static_cast<int16_t>((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]);
}

uint8_t clampAutoPwm(int value) {
  return static_cast<uint8_t>(
      constrain(value, static_cast<int>(kMotorAutoMinPwm),
                static_cast<int>(kMotorAutoMaxPwm)));
}

int16_t stepTowardCorrection(int16_t current, int16_t target) {
  if (target > current) {
    return current + 1;
  }
  if (target < current) {
    return current - 1;
  }
  return current;
}

int signedScaledCorrection(int32_t value, int32_t deadband, int32_t divisor,
                           int maxCorrection) {
  if (abs(value) <= deadband) {
    return 0;
  }

  const int sign = value > 0 ? 1 : -1;
  const int32_t magnitude = abs(value) - deadband;
  const int correction =
      static_cast<int>((magnitude + divisor / 2) / divisor);
  return constrain(sign * correction, -maxCorrection, maxCorrection);
}

void printAutoMotorOutput(const __FlashStringHelper* stage, uint8_t robotLeftPwm,
                          uint8_t robotRightPwm, int16_t correction) {
  Serial.print(F("motor gyro output stage="));
  Serial.print(stage);
  Serial.print(F(" robot_left_pwm="));
  Serial.print(robotLeftPwm);
  Serial.print(F(" robot_right_pwm="));
  Serial.print(robotRightPwm);
  Serial.print(F(" viewer_left_pwm="));
  Serial.print(robotRightPwm);
  Serial.print(F(" viewer_right_pwm="));
  Serial.print(robotLeftPwm);
  Serial.print(F(" correction="));
  Serial.print(correction);
  Serial.println(F(" note=viewer_left_is_robot_right"));
}

const __FlashStringHelper* axisName(uint8_t axis) {
  if (axis == 0) {
    return F("X");
  }
  if (axis == 1) {
    return F("Y");
  }
  return F("Z");
}

const __FlashStringHelper* wheelDriveName(WheelDrive drive) {
  switch (drive) {
    case WheelDrive::Forward:
      return F("forward");
    case WheelDrive::Reverse:
      return F("reverse");
    case WheelDrive::Brake:
      return F("brake");
    case WheelDrive::Stop:
    default:
      return F("stop");
  }
}

const char* radarGuidedPhaseName(uint8_t phase) {
  static const char* const kNames[] = {"empty_1", "wave_1", "empty_2", "wave_2", "empty_3"};
  return phase < 5 ? kNames[phase] : "done";
}

const char* radarGuidedPhaseInstruction(uint8_t phase) {
  static const char* const kInstructions[] = {
      "keep the radar facing empty space",
      "wave your hand in front of the radar",
      "keep the radar facing empty space",
      "wave your hand in front of the radar again",
      "keep the radar facing empty space",
  };
  return phase < 5 ? kInstructions[phase] : "test done";
}

bool radarGuidedPhaseExpectTarget(uint8_t phase) {
  return phase == 1 || phase == 3;
}

unsigned long radarGuidedPhaseDurationMs(uint8_t phase) {
  return radarGuidedPhaseExpectTarget(phase) ? kRadarGuidedTargetPhaseMs
                                             : kRadarGuidedEmptyPhaseMs;
}

int8_t signOf(int64_t value) {
  if (value > 0) {
    return 1;
  }
  if (value < 0) {
    return -1;
  }
  return 0;
}

bool readImuDebugFrame(ImuSensor& imu, ImuDebugFrame& frame) {
  uint8_t status[2] = {};
  uint8_t timestamp[3] = {};
  uint8_t data[kQmiTempAccelGyroReadLength] = {};

  if (!imu.readDebugRegisters(kQmiStatus0, status, sizeof(status)) ||
      !imu.readDebugRegisters(kQmiTimestampL, timestamp, sizeof(timestamp)) ||
      !imu.readDebugRegisters(kQmiTempL, data, sizeof(data))) {
    frame.ready = false;
    return false;
  }

  frame.ready = true;
  frame.status0 = status[0];
  frame.status1 = status[1];
  frame.timestamp = static_cast<uint32_t>(timestamp[0]) |
                    (static_cast<uint32_t>(timestamp[1]) << 8) |
                    (static_cast<uint32_t>(timestamp[2]) << 16);
  frame.tempRaw = readInt16Le(&data[0]);
  frame.accelRaw[0] = readInt16Le(&data[kQmiAccelXoutL - kQmiTempL + 0]);
  frame.accelRaw[1] = readInt16Le(&data[kQmiAccelXoutL - kQmiTempL + 2]);
  frame.accelRaw[2] = readInt16Le(&data[kQmiAccelXoutL - kQmiTempL + 4]);
  frame.gyroRaw[0] = readInt16Le(&data[kQmiGyroXoutL - kQmiTempL + 0]);
  frame.gyroRaw[1] = readInt16Le(&data[kQmiGyroXoutL - kQmiTempL + 2]);
  frame.gyroRaw[2] = readInt16Le(&data[kQmiGyroXoutL - kQmiTempL + 4]);
  return true;
}

bool logoTouched(const LogoTouchInput& logoTouch) {
  const int32_t delta = logoTouch.delta();
  const uint32_t absDelta =
      delta < 0 ? static_cast<uint32_t>(-delta) : static_cast<uint32_t>(delta);
  return absDelta >= logoTouch.threshold();
}

}  // namespace

HardwareSelfTestService::HardwareSelfTestService(BatteryMonitor& battery,
                                                 AudioInput& audioInput,
                                                 AudioOutput& audioOutput,
                                                 LedPixel& led,
                                                 MotorDriver& motors,
                                                 RadarSensor& radar,
                                                 FaceDisplay& faceDisplay,
                                                 ImuSensor& imu,
                                                 LogoTouchInput& logoTouch)
    : battery_(battery),
      audioInput_(audioInput),
      audioOutput_(audioOutput),
      led_(led),
      motors_(motors),
      radar_(radar),
      faceDisplay_(faceDisplay),
      imu_(imu),
      logoTouch_(logoTouch),
      audioLoopback_(audioInput, audioOutput) {}

void HardwareSelfTestService::begin() {
  motorStopAtMs_ = 0;
  manualMotorTimeoutMs_ = 0;
  autoMotorPhase_ = 0;
  autoMotorNextMs_ = 0;
  autoMotorControlNextMs_ = 0;
  autoMotorDone_ = false;
  autoMotorFailed_ = false;
  motorCalibrationNextMs_ = 0;
  motorCalibrationPhase_ = 0;
  radarGuidedActive_ = false;
  radarGuidedDone_ = false;
  radarGuidedPhase_ = 0;
  radarGuidedPhaseStartMs_ = 0;
  radarGuidedNextSampleMs_ = 0;
  radarGuidedLastSequence_ = 0;
  nextBatteryDisplayMs_ = 0;
  shippingDischargeActive_ = false;
  shippingDischargeDone_ = false;
  shippingDischargeFailed_ = false;
  shippingDischargeMotorOn_ = false;
  snprintf(shippingDischargeMessage_, sizeof(shippingDischargeMessage_), "idle");
}

void HardwareSelfTestService::update() {
  radar_.update();
  audioLoopback_.update();
  updateBatteryDisplay();
  updateShippingDischarge();
  updateRadarGuidedTest();
  updateAutoMotorCalibration();
  if (autoMotorPhase_ != 0) {
    return;
  }

  if (motorCalibrationPhase_ != 0 && motorCalibrationNextMs_ != 0 &&
      static_cast<long>(millis() - motorCalibrationNextMs_) >= 0) {
    if (motorCalibrationPhase_ == 1) {
      motors_.drive(motorCalibrationDirection_, motorCalibrationDirection_, 0,
                    motorCalibrationLeftHoldPwm_, motorCalibrationRightHoldPwm_);
      motorCalibrationPhase_ = 2;
      motorCalibrationNextMs_ = millis() + kMotorCalibrationHoldMs;
      return;
    }

    if (motorCalibrationPhase_ == 2) {
      motors_.drive(WheelDrive::Brake, WheelDrive::Brake, 0,
                    kMotorCalibrationBrakePwm, kMotorCalibrationBrakePwm);
      motorCalibrationPhase_ = 3;
      motorCalibrationNextMs_ = millis() + kMotorCalibrationBrakeMs;
      return;
    }

    stopMotorPulse();
    return;
  }

  if (motorStopAtMs_ == 0 || millis() < motorStopAtMs_) {
    if (manualMotorTimeoutMs_ == 0 || millis() < manualMotorTimeoutMs_) {
      return;
    }
  }

  stopMotorPulse();
}

HardwareDiagnosticStatus HardwareSelfTestService::status() const {
  HardwareDiagnosticStatus result;
  result.faceReady = faceDisplay_.ready();
  result.ledReady = led_.ready();
  result.audioInputReady = audioInput_.ready();
  result.audioOutputReady = audioOutput_.ready();
  result.degraded = !result.faceReady || !result.ledReady || !result.audioInputReady ||
                    !result.audioOutputReady;
  result.visualFeedbackAvailable = result.faceReady || result.ledReady;
  result.soundAvailable = result.audioOutputReady;
  result.micInputAvailable = result.audioInputReady;
  return result;
}

BatterySnapshot HardwareSelfTestService::batterySnapshot() const {
  return battery_.read();
}

RadarSnapshot HardwareSelfTestService::radarSnapshot() const {
  return radar_.read();
}

RadarTargetSnapshot HardwareSelfTestService::radarTargetSnapshot() {
  return radar_.readTarget();
}

bool HardwareSelfTestService::motorLeftInverted() const {
  return motors_.leftInverted();
}

bool HardwareSelfTestService::motorRightInverted() const {
  return motors_.rightInverted();
}

uint8_t HardwareSelfTestService::motorLeftDefaultDuty() const {
  return motors_.leftDefaultDuty();
}

uint8_t HardwareSelfTestService::motorRightDefaultDuty() const {
  return motors_.rightDefaultDuty();
}

uint8_t HardwareSelfTestService::audioVolumePercent() const {
  return audioOutput_.volumePercent();
}

bool HardwareSelfTestService::shippingDischargeActive() const {
  return shippingDischargeActive_;
}

bool HardwareSelfTestService::shippingDischargeDone() const {
  return shippingDischargeDone_;
}

bool HardwareSelfTestService::shippingDischargeFailed() const {
  return shippingDischargeFailed_;
}

uint8_t HardwareSelfTestService::shippingDischargeTargetPercent() const {
  return kShippingDischargeTargetPercent;
}

const char* HardwareSelfTestService::shippingDischargeMessage() const {
  return shippingDischargeMessage_;
}

bool HardwareSelfTestService::audioRecordingAvailable() const {
  return audioLoopback_.recordingAvailable();
}

uint32_t HardwareSelfTestService::audioRecordingSampleRateHz() const {
  return audioLoopback_.sampleRateHz();
}

const int16_t* HardwareSelfTestService::audioRecordingSamples() const {
  return audioLoopback_.recordedSamples();
}

size_t HardwareSelfTestService::audioRecordingSampleCount() const {
  return audioLoopback_.recordedSampleCount();
}

size_t HardwareSelfTestService::audioRecordingByteCount() const {
  return audioLoopback_.recordedByteCount();
}

void HardwareSelfTestService::saveMotorCalibration(bool leftInverted,
                                                   bool rightInverted,
                                                   uint8_t leftDuty,
                                                   uint8_t rightDuty) {
  motors_.saveCalibration(leftInverted, rightInverted, leftDuty, rightDuty);
}

void HardwareSelfTestService::saveAudioVolumePercent(uint8_t percent) {
  audioOutput_.setVolumePercent(percent);
}

void HardwareSelfTestService::printHelp(Print& out) const {
  out.println(F("self-test commands:"));
  out.println(F("  selftest"));
  out.println(F("  status"));
  out.println(F("  battery"));
  out.println(F("  radar"));
  out.println(F("  radar target"));
  out.println(F("  radar sample"));
  out.println(F("  radar parser selftest"));
  out.println(F("  radar guided test"));
  out.println(F("  radar guided status"));
  out.println(F("  radar guided blocking"));
  out.println(F("  radar config"));
  out.println(F("  radar sensitivity"));
  out.println(F("  radar engineering on|off"));
  out.println(F("  radar desk"));
  out.println(F("  radar near"));
  out.println(F("  radar resolution"));
  out.println(F("  radar res20"));
  out.println(F("  radar calibrate"));
  out.println(F("  radar calibrate status"));
  out.println(F("  radar factory reset"));
  out.println(F("  radar bridge"));
  out.println(F("  mic"));
  out.println(F("  mic mode"));
  out.println(F("  mic mode [0-17]"));
  out.println(F("  audio record"));
  out.println(F("  audio record status"));
  out.println(F("  audio record stop"));
  out.println(F("  audio loopback"));
  out.println(F("  audio loopback status"));
  out.println(F("  audio loopback stop"));
  out.println(F("  i2c scan"));
  out.println(F("  imu"));
  out.println(F("  imu raw test"));
  out.println(F("  logo touch"));
  out.println(F("  logo touch watch"));
  out.println(F("  speaker"));
  out.println(F("  audio volume [0-100]"));
  out.println(F("  shipping discharge start|status|stop"));
  out.println(F("  led red|green|blue|white|off"));
  out.println(F("  led pin high|low|pulse"));
  out.println(F("  motor forward|reverse|stop"));
  out.println(F("  motor diag [a|b]"));
  out.println(F("  motor manual forward [leftPwm rightPwm]"));
  out.println(F("  motor manual reverse [leftPwm rightPwm]"));
  out.println(F("  motor auto forward [basePwm]"));
  out.println(F("  motor auto status"));
  out.println(F("  motor cal forward [leftStart rightStart leftHold rightHold]"));
  out.println(F("  motor cal reverse [leftStart rightStart leftHold rightHold]"));
  out.println(F("  motor cal stop"));
}

bool HardwareSelfTestService::handleCommand(const String& command, Print& out) {
  String cmd = command;
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) {
    return false;
  }

  if (cmd == F("help") || cmd == F("?")) {
    printHelp(out);
    return true;
  }
  if (cmd == F("selftest")) {
    printReport(out);
    return true;
  }
  if (cmd == F("status")) {
    printStatus(out);
    return true;
  }
  if (cmd == F("battery")) {
    printBattery(out);
    return true;
  }
  if (cmd == F("radar")) {
    printRadar(out);
    return true;
  }
  if (cmd == F("radar target")) {
    printRadarTarget(out);
    return true;
  }
  if (cmd == F("radar sample")) {
    printRadarSamples(out);
    return true;
  }
  if (cmd == F("radar parser selftest")) {
    printRadarParserSelfTest(out);
    return true;
  }
  if (cmd == F("radar guided test") || cmd == F("radar guided") ||
      cmd == F("radar guided start")) {
    startRadarGuidedTest(out);
    return true;
  }
  if (cmd == F("radar guided status")) {
    printRadarGuidedStatus(out);
    return true;
  }
  if (cmd == F("radar guided blocking")) {
    runRadarGuidedTest(out);
    return true;
  }
  if (cmd == F("radar config")) {
    printRadarConfig(out);
    return true;
  }
  if (cmd == F("radar sensitivity")) {
    printRadarSensitivity(out);
    return true;
  }
  if (cmd == F("radar engineering on")) {
    setRadarEngineeringMode(true, out);
    return true;
  }
  if (cmd == F("radar engineering off")) {
    setRadarEngineeringMode(false, out);
    return true;
  }
  if (cmd == F("radar desk")) {
    applyRadarDeskMode(out);
    return true;
  }
  if (cmd == F("radar near")) {
    applyRadarNearConfig(out);
    return true;
  }
  if (cmd == F("radar resolution")) {
    printRadarResolution(out);
    return true;
  }
  if (cmd == F("radar res20")) {
    applyRadarResolution20cm(out);
    return true;
  }
  if (cmd == F("radar calibrate")) {
    startRadarCalibration(out);
    return true;
  }
  if (cmd == F("radar calibrate status") || cmd == F("radar calibration status")) {
    printRadarCalibrationStatus(out);
    return true;
  }
  if (cmd == F("radar factory reset")) {
    restoreRadarFactoryDefaults(out);
    return true;
  }
  if (cmd == F("radar bridge")) {
    startRadarBridge(out);
    return true;
  }
  if (cmd == F("mic")) {
    printMic(out);
    return true;
  }
  if (cmd == F("mic mode")) {
    printMicMode(out);
    return true;
  }
  if (cmd.startsWith(F("mic mode "))) {
    setMicMode(cmd, out);
    return true;
  }
  if (cmd == F("audio record") || cmd == F("audio loopback")) {
    startAudioLoopback(out);
    return true;
  }
  if (cmd == F("audio record status") || cmd == F("audio loopback status")) {
    printAudioLoopbackStatus(out);
    return true;
  }
  if (cmd == F("audio record stop") || cmd == F("audio loopback stop")) {
    stopAudioLoopback(out);
    return true;
  }
  if (cmd == F("i2c scan")) {
    printI2cScan(out);
    return true;
  }
  if (cmd == F("imu")) {
    printImu(out);
    return true;
  }
  if (cmd == F("imu raw test")) {
    printImuRawTest(out);
    return true;
  }
  if (cmd == F("logo touch")) {
    printLogoTouch(out);
    return true;
  }
  if (cmd == F("logo touch watch")) {
    watchLogoTouch(out);
    return true;
  }
  if (cmd == F("speaker")) {
    testSpeaker(out);
    return true;
  }
  if (cmd == F("audio volume")) {
    printAudioVolume(out);
    return true;
  }
  if (cmd.startsWith(F("audio volume "))) {
    const int space = cmd.lastIndexOf(' ');
    if (space > 0 && space < static_cast<int>(cmd.length() - 1)) {
      const int value = cmd.substring(space + 1).toInt();
      if (value >= 0 && value <= 100) {
        saveAudioVolumePercent(static_cast<uint8_t>(value));
      }
    }
    printAudioVolume(out);
    return true;
  }
  if (cmd == F("shipping discharge") ||
      cmd == F("shipping discharge start")) {
    startShippingDischarge(out);
    return true;
  }
  if (cmd == F("shipping discharge status")) {
    printShippingDischargeStatus(out);
    return true;
  }
  if (cmd == F("shipping discharge stop")) {
    stopShippingDischarge(out);
    return true;
  }
  if (cmd == F("led red")) {
    testLed({255, 0, 0}, out);
    return true;
  }
  if (cmd == F("led green")) {
    testLed({0, 255, 0}, out);
    return true;
  }
  if (cmd == F("led blue")) {
    testLed({0, 0, 255}, out);
    return true;
  }
  if (cmd == F("led white")) {
    testLed({64, 64, 64}, out);
    return true;
  }
  if (cmd == F("led off")) {
    led_.off();
    out.println(F("led off"));
    return true;
  }
  if (cmd == F("led pin high")) {
    led_.off();
    pinMode(pins::LED_DATA, OUTPUT);
    digitalWrite(pins::LED_DATA, HIGH);
    out.println(F("led data pin high"));
    return true;
  }
  if (cmd == F("led pin low")) {
    led_.off();
    pinMode(pins::LED_DATA, OUTPUT);
    digitalWrite(pins::LED_DATA, LOW);
    out.println(F("led data pin low"));
    return true;
  }
  if (cmd == F("led pin pulse")) {
    pulseLedDataPin(out);
    return true;
  }
  if (cmd == F("motor forward")) {
    testMotor(WheelDrive::Forward, WheelDrive::Forward, out);
    return true;
  }
  if (cmd == F("motor diag") || cmd == F("motor diagnostic")) {
    runMotorDriverDiagnostic(out, true, true);
    return true;
  }
  if (cmd == F("motor diag a") || cmd == F("motor diagnostic a")) {
    runMotorDriverDiagnostic(out, true, false);
    return true;
  }
  if (cmd == F("motor diag b") || cmd == F("motor diagnostic b")) {
    runMotorDriverDiagnostic(out, false, true);
    return true;
  }
  if (cmd == F("motor reverse")) {
    testMotor(WheelDrive::Reverse, WheelDrive::Reverse, out);
    return true;
  }
  if (cmd == F("motor stop")) {
    if (shippingDischargeActive_) {
      finishShippingDischarge(false, "stopped by motor stop", false);
    }
    stopMotorPulse();
    out.println(F("motor stop"));
    return true;
  }
  if (cmd == F("motor manual forward") || cmd.startsWith(F("motor manual forward "))) {
    startManualMotor(WheelDrive::Forward, cmd, out);
    return true;
  }
  if (cmd == F("motor manual reverse") || cmd.startsWith(F("motor manual reverse "))) {
    startManualMotor(WheelDrive::Reverse, cmd, out);
    return true;
  }
  if (cmd == F("motor auto forward") || cmd.startsWith(F("motor auto forward "))) {
    startAutoMotorCalibration(WheelDrive::Forward, cmd, out);
    return true;
  }
  if (cmd == F("motor auto status")) {
    printAutoMotorCalibration(out);
    return true;
  }
  if (cmd == F("motor cal forward") || cmd.startsWith(F("motor cal forward "))) {
    startMotorCalibration(WheelDrive::Forward, cmd, out);
    return true;
  }
  if (cmd == F("motor cal reverse") || cmd.startsWith(F("motor cal reverse "))) {
    startMotorCalibration(WheelDrive::Reverse, cmd, out);
    return true;
  }
  if (cmd == F("motor cal stop")) {
    stopMotorPulse();
    out.println(F("motor calibration stop"));
    return true;
  }
  return false;
}

void HardwareSelfTestService::printReport(Print& out) {
  out.println(F("self-test report begin"));
  printStatus(out);
  printBattery(out);
  printRadar(out);
  printMic(out);
  out.println(F("self-test report end"));
}

void HardwareSelfTestService::printStatus(Print& out) const {
  const HardwareDiagnosticStatus snapshot = status();
  out.println(F("hardware:"));
  printBool(out, F("  face="), snapshot.faceReady);
  printBool(out, F("  led="), snapshot.ledReady);
  printBool(out, F("  audio_input="), snapshot.audioInputReady);
  printBool(out, F("  audio_output="), snapshot.audioOutputReady);
  printBool(out, F("  degraded="), snapshot.degraded);
  printBool(out, F("  web_available="), snapshot.webAvailable);
  printBool(out, F("  visual_feedback_available="), snapshot.visualFeedbackAvailable);
  printBool(out, F("  sound_available="), snapshot.soundAvailable);
  printBool(out, F("  mic_input_available="), snapshot.micInputAvailable);
}

void HardwareSelfTestService::printBattery(Print& out) const {
  const BatterySnapshot snapshot = battery_.read();
  out.println(F("battery:"));
  printBool(out, F("  usb="), snapshot.usbPresent);
  printBool(out, F("  charging="), snapshot.charging);
  printBool(out, F("  standby="), snapshot.standby);
  out.print(F("  raw_adc="));
  out.println(snapshot.rawAdc);
  out.print(F("  voltage_mv="));
  out.println(snapshot.voltageMv);
  out.print(F("  percent_estimate="));
  out.println(snapshot.percent);
}

void HardwareSelfTestService::updateBatteryDisplay() {
  if (!faceDisplay_.ready()) {
    return;
  }
  if (nextBatteryDisplayMs_ != 0 &&
      static_cast<long>(millis() - nextBatteryDisplayMs_) < 0) {
    return;
  }

  const BatterySnapshot snapshot = battery_.read();
  faceDisplay_.showBattery(snapshot.percent, snapshot.voltageMv,
                           snapshot.usbPresent, snapshot.charging,
                           snapshot.standby);
  nextBatteryDisplayMs_ = millis() + kBatteryDisplayIntervalMs;
}

void HardwareSelfTestService::updateShippingDischarge() {
  if (!shippingDischargeActive_) {
    return;
  }

  const BatterySnapshot snapshot = battery_.read();
  if (snapshot.usbPresent) {
    finishShippingDischarge(true, "usb plugged during discharge", false);
    return;
  }

  if (snapshot.percent <= kShippingDischargeTargetPercent) {
    finishShippingDischarge(false, "target reached", true);
    return;
  }

  if (millis() - shippingDischargeStartedMs_ >= kShippingDischargeMaxMs) {
    finishShippingDischarge(true, "timeout before target", true);
    return;
  }

  const unsigned long now = millis();
  if (!shippingDischargeMotorOn_) {
    if (shippingDischargePhaseStartedMs_ != 0 &&
        now - shippingDischargePhaseStartedMs_ < kShippingDischargeRestMs) {
      return;
    }

    motors_.drive(WheelDrive::Forward, WheelDrive::Forward, 0,
                  kShippingDischargePwm, kShippingDischargePwm);
    shippingDischargeMotorOn_ = true;
    shippingDischargePhaseStartedMs_ = now;
    snprintf(shippingDischargeMessage_, sizeof(shippingDischargeMessage_),
             "running motor discharge");
    return;
  }

  if (now - shippingDischargePhaseStartedMs_ >= kShippingDischargeOnMs) {
    motors_.stop();
    shippingDischargeMotorOn_ = false;
    shippingDischargePhaseStartedMs_ = now;
    snprintf(shippingDischargeMessage_, sizeof(shippingDischargeMessage_),
             "resting before next voltage check");
  }
}

void HardwareSelfTestService::finishShippingDischarge(bool failed,
                                                      const char* message,
                                                      bool alarm) {
  motors_.stop();
  shippingDischargeActive_ = false;
  shippingDischargeDone_ = true;
  shippingDischargeFailed_ = failed;
  shippingDischargeMotorOn_ = false;
  snprintf(shippingDischargeMessage_, sizeof(shippingDischargeMessage_), "%s",
           message);

  if (alarm) {
    for (uint8_t index = 0; index < 3; ++index) {
      audioOutput_.playTestTone(kShippingDischargeAlarmHz,
                                kShippingDischargeAlarmMs);
      delay(120);
    }
  }

  Serial.print(F("shipping discharge finish failed="));
  Serial.print(failed ? F("1") : F("0"));
  Serial.print(F(" message="));
  Serial.println(message);
}

void HardwareSelfTestService::printRadar(Print& out) const {
  const RadarSnapshot snapshot = radar_.read();
  out.println(F("radar:"));
  printBool(out, F("  occupied="), snapshot.occupied);
  printBool(out, F("  rx_level="), snapshot.rxLevelHigh);
}

void HardwareSelfTestService::printRadarTarget(Print& out) {
  const RadarTargetSnapshot snapshot = radar_.readTarget();
  out.println(F("radar target:"));
  printBool(out, F("  received="), snapshot.received);
  out.print(F("  frame_age_ms="));
  out.println(snapshot.frameAgeMs);
  out.print(F("  valid_frames="));
  out.println(snapshot.validFrameCount);
  out.print(F("  invalid_frames="));
  out.println(snapshot.invalidFrameCount);
  if (!snapshot.received) {
    return;
  }

  out.print(F("  state="));
  out.println(snapshot.targetState);
  printBool(out, F("  engineering_mode="), snapshot.engineeringMode);
  out.print(F("  raw_payload="));
  for (uint8_t i = 0; i < snapshot.rawPayloadLength; ++i) {
    if (i > 0) {
      out.print(' ');
    }
    printHexByte(out, snapshot.rawPayload[i]);
  }
  out.println();
  printBool(out, F("  has_target="), snapshot.hasTarget);
  printBool(out, F("  state_target="), snapshot.stateTarget);
  printBool(out, F("  energy_target="), snapshot.energyTarget);
  printBool(out, F("  moving_gate_target="), snapshot.movingGateTarget);
  out.print(F("  target_confidence="));
  out.println(snapshot.targetConfidence);
  out.println(F("  distance_note=reported_cm_not_precision_ruler"));
  out.print(F("  target_distance_cm="));
  out.println(snapshot.targetDistanceCm);
  out.print(F("  moving_distance_cm="));
  out.println(snapshot.movingDistanceCm);
  out.print(F("  moving_energy="));
  out.println(snapshot.movingEnergy);
  out.print(F("  static_distance_cm="));
  out.println(snapshot.staticDistanceCm);
  out.print(F("  static_energy="));
  out.println(snapshot.staticEnergy);
  if (snapshot.engineeringMode) {
    out.print(F("  moving_gate_energy="));
    for (uint8_t gate = 0; gate < 14; ++gate) {
      if (gate > 0) {
        out.print(',');
      }
      out.print(snapshot.movingGateEnergy[gate]);
    }
    out.println();
    out.print(F("  static_gate_energy="));
    for (uint8_t gate = 0; gate < 14; ++gate) {
      if (gate > 0) {
        out.print(',');
      }
      out.print(snapshot.staticGateEnergy[gate]);
    }
    out.println();
    out.print(F("  light_level="));
    out.println(snapshot.lightLevel);
  }
}

void HardwareSelfTestService::printRadarSamples(Print& out) {
  uint16_t samples = 0;
  uint16_t noTargetFrames = 0;
  uint16_t movingTargetFrames = 0;
  uint16_t staticTargetFrames = 0;
  uint16_t combinedTargetFrames = 0;
  uint16_t minMovingDistanceCm = 0;
  uint16_t maxMovingDistanceCm = 0;
  uint16_t minStaticDistanceCm = 0;
  uint16_t maxStaticDistanceCm = 0;
  uint8_t maxMovingEnergy = 0;
  uint8_t maxStaticEnergy = 0;
  const RadarTargetSnapshot before = radar_.readTarget();
  uint32_t lastSequence = radar_.readTarget().sequence;
  const unsigned long deadlineMs = millis() + 3000;
  while (static_cast<long>(millis() - deadlineMs) < 0) {
    radar_.update();
    const RadarTargetSnapshot snapshot = radar_.readTarget();
    if (!snapshot.received || snapshot.sequence == lastSequence) {
      delay(5);
      continue;
    }
    lastSequence = snapshot.sequence;
    ++samples;
    if (snapshot.targetState == 0) {
      ++noTargetFrames;
    }
    if ((snapshot.targetState & 0x01) != 0) {
      ++movingTargetFrames;
    }
    if ((snapshot.targetState & 0x02) != 0) {
      ++staticTargetFrames;
    }
    if ((snapshot.targetState & 0x03) == 0x03) {
      ++combinedTargetFrames;
    }

    if (snapshot.movingDistanceCm > 0) {
      if (minMovingDistanceCm == 0 || snapshot.movingDistanceCm < minMovingDistanceCm) {
        minMovingDistanceCm = snapshot.movingDistanceCm;
      }
      maxMovingDistanceCm = max(maxMovingDistanceCm, snapshot.movingDistanceCm);
    }
    if (snapshot.staticDistanceCm > 0) {
      if (minStaticDistanceCm == 0 || snapshot.staticDistanceCm < minStaticDistanceCm) {
        minStaticDistanceCm = snapshot.staticDistanceCm;
      }
      maxStaticDistanceCm = max(maxStaticDistanceCm, snapshot.staticDistanceCm);
    }
    maxMovingEnergy = max(maxMovingEnergy, snapshot.movingEnergy);
    maxStaticEnergy = max(maxStaticEnergy, snapshot.staticEnergy);
  }

  const RadarTargetSnapshot after = radar_.readTarget();
  out.println(F("radar sample:"));
  out.println(F("  duration_ms=3000"));
  out.println(F("  distance_note=reported_cm_not_precision_ruler"));
  out.print(F("  unique_frames="));
  out.println(samples);
  out.print(F("  no_target_frames="));
  out.println(noTargetFrames);
  out.print(F("  moving_target_frames="));
  out.println(movingTargetFrames);
  out.print(F("  static_target_frames="));
  out.println(staticTargetFrames);
  out.print(F("  combined_target_frames="));
  out.println(combinedTargetFrames);
  out.print(F("  moving_distance_cm_min="));
  out.println(minMovingDistanceCm);
  out.print(F("  moving_distance_cm_max="));
  out.println(maxMovingDistanceCm);
  out.print(F("  static_distance_cm_min="));
  out.println(minStaticDistanceCm);
  out.print(F("  static_distance_cm_max="));
  out.println(maxStaticDistanceCm);
  out.print(F("  max_moving_energy="));
  out.println(maxMovingEnergy);
  out.print(F("  max_static_energy="));
  out.println(maxStaticEnergy);
  out.print(F("  valid_frame_delta="));
  out.println(after.validFrameCount - before.validFrameCount);
  out.print(F("  invalid_frame_delta="));
  out.println(after.invalidFrameCount - before.invalidFrameCount);
}

void HardwareSelfTestService::printRadarParserSelfTest(Print& out) const {
  const RadarParserSelfTestResult result = radar_.parserSelfTest();
  out.println(F("radar parser selftest:"));
  printBool(out, F("  normal_frame_accepted="), result.normalFrameAccepted);
  printBool(out, F("  static_target_accepted="), result.staticTargetAccepted);
  printBool(out, F("  clear_frame_accepted="), result.clearFrameAccepted);
  printBool(out, F("  corrupt_frame_rejected="), result.corruptFrameRejected);
  printBool(out, F("  passed="), result.passed());
}

void HardwareSelfTestService::runRadarGuidedTest(Print& out) {
  struct Phase {
    const char* name;
    const char* instruction;
    bool expectTarget;
  };

  const Phase phases[] = {
      {"empty_1", "keep the radar facing empty space", false},
      {"wave_1", "wave your hand in front of the radar", true},
      {"empty_2", "keep the radar facing empty space", false},
      {"wave_2", "wave your hand in front of the radar again", true},
      {"empty_3", "keep the radar facing empty space", false},
  };

  out.println(F("radar guided test:"));
  out.println(F("  rule=follow each phase exactly when it starts"));
  out.print(F("  phase_ms="));
  out.println(kRadarGuidedBlockingPhaseMs);
  out.println(F("  final_target_uses=serial_target_not_out_pin"));

  for (uint8_t phaseIndex = 0; phaseIndex < sizeof(phases) / sizeof(phases[0]);
       ++phaseIndex) {
    const Phase& phase = phases[phaseIndex];
    RadarGuidedPhaseStats stats;
    const unsigned long phaseStartMs = millis();

    out.print(F("phase_start name="));
    out.print(phase.name);
    out.print(F(" instruction=\""));
    out.print(phase.instruction);
    out.println(F("\""));

    while (millis() - phaseStartMs < kRadarGuidedBlockingPhaseMs) {
      radar_.update();
      const RadarSnapshot pinSnapshot = radar_.read();
      const RadarTargetSnapshot target = radar_.readTarget();
      const uint16_t elapsedMs =
          static_cast<uint16_t>(min(millis() - phaseStartMs, 65535UL));

      ++stats.samples;
      if (pinSnapshot.occupied) {
        ++stats.occupiedFrames;
      }
      if (target.received) {
        ++stats.receivedFrames;
        stats.lastState = target.targetState;
        stats.maxMovingEnergy = max(stats.maxMovingEnergy, target.movingEnergy);
        stats.maxStaticEnergy = max(stats.maxStaticEnergy, target.staticEnergy);
        stats.maxConfidence = max(stats.maxConfidence, target.targetConfidence);

        if (target.stateTarget) {
          ++stats.stateTargetFrames;
        }
        if (target.energyTarget) {
          ++stats.energyTargetFrames;
        }
        if (target.hasTarget) {
          ++stats.targetFrames;
          if (stats.firstTargetMs == 0) {
            stats.firstTargetMs = elapsedMs;
          }
          if (target.targetDistanceCm > 0 &&
              (stats.minTargetDistanceCm == 0 ||
               target.targetDistanceCm < stats.minTargetDistanceCm)) {
            stats.minTargetDistanceCm = target.targetDistanceCm;
          }
        }
      }

      delay(kRadarGuidedSamplePauseMs);
    }

    out.print(F("phase_result name="));
    out.print(phase.name);
    out.print(F(" expected="));
    out.print(phase.expectTarget ? F("target") : F("empty"));
    out.print(F(" samples="));
    out.print(stats.samples);
    out.print(F(" received="));
    out.print(stats.receivedFrames);
    out.print(F(" target_frames="));
    out.print(stats.targetFrames);
    out.print(F(" state_target_frames="));
    out.print(stats.stateTargetFrames);
    out.print(F(" energy_target_frames="));
    out.print(stats.energyTargetFrames);
    out.print(F(" out_high_frames="));
    out.print(stats.occupiedFrames);
    out.print(F(" first_target_ms="));
    out.print(stats.firstTargetMs);
    out.print(F(" min_target_cm="));
    out.print(stats.minTargetDistanceCm);
    out.print(F(" max_moving_energy="));
    out.print(stats.maxMovingEnergy);
    out.print(F(" max_static_energy="));
    out.print(stats.maxStaticEnergy);
    out.print(F(" max_confidence="));
    out.print(stats.maxConfidence);
    out.print(F(" last_state="));
    out.print(stats.lastState);
    out.print(F(" result="));
    if (phase.expectTarget) {
      out.println(stats.targetFrames > 0 ? F("detected") : F("missed"));
    } else {
      out.println(stats.targetFrames == 0 ? F("clear") : F("false_positive"));
    }
  }

  out.println(F("radar guided test done"));
}

void HardwareSelfTestService::startRadarGuidedTest(Print& out) {
  memset(radarGuidedSamples_, 0, sizeof(radarGuidedSamples_));
  memset(radarGuidedReceivedFrames_, 0, sizeof(radarGuidedReceivedFrames_));
  memset(radarGuidedTargetFrames_, 0, sizeof(radarGuidedTargetFrames_));
  memset(radarGuidedStateTargetFrames_, 0, sizeof(radarGuidedStateTargetFrames_));
  memset(radarGuidedEnergyTargetFrames_, 0, sizeof(radarGuidedEnergyTargetFrames_));
  memset(radarGuidedOutHighFrames_, 0, sizeof(radarGuidedOutHighFrames_));
  memset(radarGuidedFirstTargetMs_, 0, sizeof(radarGuidedFirstTargetMs_));
  memset(radarGuidedMinTargetDistanceCm_, 0, sizeof(radarGuidedMinTargetDistanceCm_));
  memset(radarGuidedMaxMovingEnergy_, 0, sizeof(radarGuidedMaxMovingEnergy_));
  memset(radarGuidedMaxStaticEnergy_, 0, sizeof(radarGuidedMaxStaticEnergy_));
  memset(radarGuidedMaxConfidence_, 0, sizeof(radarGuidedMaxConfidence_));
  memset(radarGuidedLastState_, 0, sizeof(radarGuidedLastState_));

  radarGuidedActive_ = true;
  radarGuidedDone_ = false;
  radarGuidedPhase_ = 0;
  radarGuidedPhaseStartMs_ = millis();
  radarGuidedNextSampleMs_ = millis();
  radarGuidedLastSequence_ = radar_.readTarget().sequence;

  out.println(F("radar guided test started"));
  out.println(F("  empty_phase_ms=7000"));
  out.println(F("  target_phase_ms=4000"));
  out.print(F("  current_phase="));
  out.println(radarGuidedPhaseName(radarGuidedPhase_));
  out.print(F("  instruction="));
  out.println(radarGuidedPhaseInstruction(radarGuidedPhase_));
}

void HardwareSelfTestService::printRadarGuidedStatus(Print& out) const {
  out.println(F("radar guided status:"));
  printBool(out, F("  active="), radarGuidedActive_);
  printBool(out, F("  done="), radarGuidedDone_);
  out.print(F("  phase="));
  out.println(radarGuidedPhaseName(radarGuidedPhase_));
  if (radarGuidedActive_) {
    out.print(F("  instruction="));
    out.println(radarGuidedPhaseInstruction(radarGuidedPhase_));
    out.print(F("  elapsed_ms="));
    out.println(millis() - radarGuidedPhaseStartMs_);
    out.print(F("  phase_duration_ms="));
    out.println(radarGuidedPhaseDurationMs(radarGuidedPhase_));
  }

  const uint8_t phaseCount = radarGuidedDone_ ? 5 : min<uint8_t>(radarGuidedPhase_ + 1, 5);
  for (uint8_t phase = 0; phase < phaseCount; ++phase) {
    out.print(F("  phase_result name="));
    out.print(radarGuidedPhaseName(phase));
    out.print(F(" expected="));
    out.print(radarGuidedPhaseExpectTarget(phase) ? F("target") : F("empty"));
    out.print(F(" samples="));
    out.print(radarGuidedSamples_[phase]);
    out.print(F(" received="));
    out.print(radarGuidedReceivedFrames_[phase]);
    out.print(F(" target_frames="));
    out.print(radarGuidedTargetFrames_[phase]);
    out.print(F(" state_target_frames="));
    out.print(radarGuidedStateTargetFrames_[phase]);
    out.print(F(" energy_target_frames="));
    out.print(radarGuidedEnergyTargetFrames_[phase]);
    out.print(F(" out_high_frames="));
    out.print(radarGuidedOutHighFrames_[phase]);
    out.print(F(" first_target_ms="));
    out.print(radarGuidedFirstTargetMs_[phase]);
    out.print(F(" min_target_cm="));
    out.print(radarGuidedMinTargetDistanceCm_[phase]);
    out.print(F(" max_moving_energy="));
    out.print(radarGuidedMaxMovingEnergy_[phase]);
    out.print(F(" max_static_energy="));
    out.print(radarGuidedMaxStaticEnergy_[phase]);
    out.print(F(" max_confidence="));
    out.print(radarGuidedMaxConfidence_[phase]);
    out.print(F(" last_state="));
    out.print(radarGuidedLastState_[phase]);
    out.print(F(" result="));
    if (radarGuidedReceivedFrames_[phase] == 0) {
      out.println(F("no_data"));
    } else if (radarGuidedPhaseExpectTarget(phase)) {
      out.println(radarGuidedTargetFrames_[phase] > 0 ? F("detected") : F("missed"));
    } else {
      out.println(radarGuidedLastState_[phase] == 0 ? F("clear") : F("false_positive"));
    }
  }
}

void HardwareSelfTestService::updateRadarGuidedTest() {
  if (!radarGuidedActive_ || radarGuidedPhase_ >= 5) {
    return;
  }

  const unsigned long now = millis();
  if (now - radarGuidedPhaseStartMs_ >= radarGuidedPhaseDurationMs(radarGuidedPhase_)) {
    ++radarGuidedPhase_;
    if (radarGuidedPhase_ >= 5) {
      radarGuidedActive_ = false;
      radarGuidedDone_ = true;
      return;
    }
    radarGuidedPhaseStartMs_ = now;
    radarGuidedNextSampleMs_ = now;
    Serial.print(F("radar guided phase_start name="));
    Serial.print(radarGuidedPhaseName(radarGuidedPhase_));
    Serial.print(F(" instruction=\""));
    Serial.print(radarGuidedPhaseInstruction(radarGuidedPhase_));
    Serial.println(F("\""));
    return;
  }

  if (static_cast<long>(now - radarGuidedNextSampleMs_) < 0) {
    return;
  }
  radarGuidedNextSampleMs_ = now + kRadarGuidedSamplePauseMs;

  const uint8_t phase = radarGuidedPhase_;
  const RadarSnapshot pinSnapshot = radar_.read();
  const RadarTargetSnapshot target = radar_.readTarget();
  const uint16_t elapsedMs =
      static_cast<uint16_t>(min(now - radarGuidedPhaseStartMs_, 65535UL));

  ++radarGuidedSamples_[phase];
  if (pinSnapshot.occupied) {
    ++radarGuidedOutHighFrames_[phase];
  }
  if (!target.received) {
    return;
  }
  if (target.sequence == radarGuidedLastSequence_) {
    return;
  }
  radarGuidedLastSequence_ = target.sequence;

  ++radarGuidedReceivedFrames_[phase];
  radarGuidedLastState_[phase] = target.targetState;
  radarGuidedMaxMovingEnergy_[phase] =
      max(radarGuidedMaxMovingEnergy_[phase], target.movingEnergy);
  radarGuidedMaxStaticEnergy_[phase] =
      max(radarGuidedMaxStaticEnergy_[phase], target.staticEnergy);
  radarGuidedMaxConfidence_[phase] =
      max(radarGuidedMaxConfidence_[phase], target.targetConfidence);

  if (target.stateTarget) {
    ++radarGuidedStateTargetFrames_[phase];
  }
  if (target.energyTarget) {
    ++radarGuidedEnergyTargetFrames_[phase];
  }
  if (!target.hasTarget) {
    return;
  }

  ++radarGuidedTargetFrames_[phase];
  if (radarGuidedFirstTargetMs_[phase] == 0) {
    radarGuidedFirstTargetMs_[phase] = elapsedMs;
  }
  if (target.targetDistanceCm > 0 &&
      (radarGuidedMinTargetDistanceCm_[phase] == 0 ||
       target.targetDistanceCm < radarGuidedMinTargetDistanceCm_[phase])) {
    radarGuidedMinTargetDistanceCm_[phase] = target.targetDistanceCm;
  }
}

void HardwareSelfTestService::printRadarConfig(Print& out) {
  const RadarBasicConfig config = radar_.readBasicConfig();
  out.println(F("radar config:"));
  printBool(out, F("  received="), config.received);
  printBool(out, F("  success="), config.success);
  if (!config.received || !config.success) {
    return;
  }

  out.print(F("  min_gate="));
  out.println(config.minGate);
  out.print(F("  max_gate="));
  out.println(config.maxGate);
  out.print(F("  absence_hold_s="));
  out.println(config.absenceHoldSeconds);
  out.print(F("  out_polarity="));
  out.println(config.outputPolarity);
  out.println(config.outputPolarity == 0 ? F("  out_meaning=occupied_high")
                                         : F("  out_meaning=occupied_low"));
}

void HardwareSelfTestService::printRadarResolution(Print& out) {
  const RadarResolutionConfig config = radar_.readResolutionConfig();
  out.println(F("radar resolution:"));
  printBool(out, F("  received="), config.received);
  printBool(out, F("  success="), config.success);
  if (!config.received || !config.success) {
    return;
  }

  out.print(F("  resolution_value="));
  out.println(config.resolutionValue);
  out.print(F("  gate_cm="));
  out.println(config.gateCentimeters);
}

void HardwareSelfTestService::printRadarSensitivity(Print& out) {
  const RadarSensitivityConfig motion = radar_.readMotionSensitivity();
  const RadarSensitivityConfig still = radar_.readStaticSensitivity();
  out.println(F("radar sensitivity:"));
  printBool(out, F("  motion_received="), motion.received);
  printBool(out, F("  motion_success="), motion.success);
  printBool(out, F("  static_received="), still.received);
  printBool(out, F("  static_success="), still.success);
  if (!motion.success || !still.success) {
    return;
  }

  out.print(F("  motion_gates="));
  for (uint8_t gate = 0; gate < 14; ++gate) {
    if (gate > 0) {
      out.print(',');
    }
    out.print(motion.gates[gate]);
  }
  out.println();

  out.print(F("  static_gates="));
  for (uint8_t gate = 0; gate < 14; ++gate) {
    if (gate > 0) {
      out.print(',');
    }
    out.print(still.gates[gate]);
  }
  out.println();
}

void HardwareSelfTestService::setRadarEngineeringMode(bool enabled, Print& out) {
  const RadarCommandResult result = radar_.setEngineeringMode(enabled);
  out.println(F("radar engineering mode:"));
  out.print(F("  requested="));
  out.println(enabled ? F("on") : F("off"));
  printBool(out, F("  received="), result.received);
  printBool(out, F("  success="), result.success);
}

void HardwareSelfTestService::applyRadarNearConfig(Print& out) {
  const bool ok = radar_.applyNearDeskConfig();
  out.println(ok ? F("radar near config applied") : F("radar near config failed"));
  if (ok) {
    printRadarConfig(out);
  }
}

void HardwareSelfTestService::applyRadarDeskMode(Print& out) {
  const RadarDeskConfigResult result = radar_.applyDeskMode();
  out.println(F("radar desk mode:"));
  printBool(out, F("  resolution_received="), result.resolution.received);
  printBool(out, F("  resolution_success="), result.resolution.success);
  printBool(out, F("  basic_received="), result.basicConfig.received);
  printBool(out, F("  basic_success="), result.basicConfig.success);
  printBool(out, F("  reboot_received="), result.reboot.received);
  printBool(out, F("  reboot_success="), result.reboot.success);
  if (result.resolution.success && result.basicConfig.success) {
    out.println(F("  expected_resolution=20cm_per_gate"));
    out.println(F("  expected_min_gate=1"));
    out.println(F("  expected_max_gate=13"));
    out.println(F("  expected_absence_hold_s=5"));
    out.println(F("  expected_out_meaning=occupied_high"));
    out.println(F("  note=desk mode written, run radar config and radar resolution to verify"));
  }
}

void HardwareSelfTestService::applyRadarResolution20cm(Print& out) {
  const RadarCommandResult result = radar_.applyResolution20cm();
  out.println(F("radar res20:"));
  printBool(out, F("  received="), result.received);
  printBool(out, F("  success="), result.success);
  if (result.success) {
    out.println(F("  note=radar module rebooting, wait 3 seconds then run radar resolution"));
  }
}

void HardwareSelfTestService::startRadarCalibration(Print& out) {
  const RadarCommandResult result = radar_.startBackgroundCalibration();
  out.println(F("radar calibrate:"));
  printBool(out, F("  received="), result.received);
  printBool(out, F("  success="), result.success);
  if (result.success) {
    out.println(F("  note=keep the radar facing empty space; wait 10 seconds, then query status"));
  }
}

void HardwareSelfTestService::printRadarCalibrationStatus(Print& out) {
  const RadarCalibrationStatus status = radar_.readBackgroundCalibrationStatus();
  out.println(F("radar calibrate status:"));
  printBool(out, F("  received="), status.received);
  printBool(out, F("  success="), status.success);
  if (!status.received || !status.success) {
    return;
  }
  printBool(out, F("  running="), status.running);
  out.print(F("  raw_status="));
  out.println(status.rawStatus);
}

void HardwareSelfTestService::restoreRadarFactoryDefaults(Print& out) {
  const RadarCommandResult result = radar_.restoreFactoryDefaults();
  out.println(F("radar factory reset:"));
  printBool(out, F("  received="), result.received);
  printBool(out, F("  success="), result.success);
  if (result.success) {
    out.println(F("  note=radar module rebooting with factory defaults"));
    out.println(F("  next=wait 3 seconds, then run radar desk"));
  }
}

void HardwareSelfTestService::startRadarBridge(Print& out) {
  if (&out != static_cast<Print*>(&Serial)) {
    out.println(F("radar bridge is serial-only"));
    out.println(F("send 'radar bridge' from USB serial, then use the HLK tool on the same COM port"));
    return;
  }

  motors_.stop();
  led_.off();
  out.println(F("radar bridge mode"));
  out.println(F("close this serial monitor, open HLK tool on this COM port at 115200"));
  out.println(F("press RESET to exit bridge mode"));
  Serial.flush();
  delay(300);
  radar_.bridge(Serial);
}

void HardwareSelfTestService::printMic(Print& out) {
  if (audioLoopback_.active()) {
    out.println(F("mic test unavailable while audio loopback is active"));
    printAudioLoopbackStatus(out);
    return;
  }

  const AudioInputSnapshot snapshot = audioInput_.readLevel();
  out.println(F("mic:"));
  out.print(F("  mode="));
  out.println(audioInput_.modeIndex());
  printBool(out, F("  ready="), snapshot.ready);
  printBool(out, F("  has_samples="), snapshot.hasSamples);
  out.print(F("  samples="));
  out.println(snapshot.samplesRead);
  out.print(F("  mean="));
  out.println(snapshot.mean);
  out.print(F("  min="));
  out.println(snapshot.minSample);
  out.print(F("  max="));
  out.println(snapshot.maxSample);
  out.print(F("  peak="));
  out.println(snapshot.peak);
  out.print(F("  avg_abs="));
  out.println(snapshot.averageAbs);
}

void HardwareSelfTestService::printMicMode(Print& out) const {
  audioInput_.printMode(out);
}

void HardwareSelfTestService::setMicMode(const String& command, Print& out) {
  if (audioLoopback_.active()) {
    out.println(F("mic mode unavailable while audio record is active"));
    printAudioLoopbackStatus(out);
    return;
  }

  const int space = command.lastIndexOf(' ');
  if (space <= 0 || space >= static_cast<int>(command.length() - 1)) {
    printMicMode(out);
    return;
  }

  const int value = command.substring(space + 1).toInt();
  if (value < 0 || value >= audioInput_.modeCount()) {
    out.print(F("mic mode invalid index="));
    out.println(value);
    out.print(F("valid_range=0-"));
    out.println(audioInput_.modeCount() - 1);
    return;
  }

  audioInput_.setMode(static_cast<uint8_t>(value), out);
}

void HardwareSelfTestService::startAudioLoopback(Print& out) {
  audioLoopback_.start(out);
}

void HardwareSelfTestService::printAudioLoopbackStatus(Print& out) const {
  audioLoopback_.printStatus(out);
}

void HardwareSelfTestService::stopAudioLoopback(Print& out) {
  audioLoopback_.stop(out);
}

void HardwareSelfTestService::printI2cScan(Print& out) {
  out.println(F("i2c scan:"));
  uint8_t count = 0;
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error != 0) {
      continue;
    }
    out.print(F("  found=0x"));
    if (address < 0x10) {
      out.print('0');
    }
    out.println(address, HEX);
    ++count;
  }
  out.print(F("  count="));
  out.println(count);
}

void HardwareSelfTestService::printImu(Print& out) {
  out.println(F("imu:"));
  printBool(out, F("  ready="), imu_.ready());
  if (!imu_.ready()) {
    return;
  }

  out.print(F("  address=0x"));
  out.println(imu_.address(), HEX);
  out.print(F("  who=0x"));
  out.println(imu_.whoAmI(), HEX);
  const ImuGyroSnapshot gyro = imu_.readGyro();
  printBool(out, F("  gyro_ready="), gyro.ready);
  out.print(F("  gyro_x_raw="));
  out.println(gyro.xRaw);
  out.print(F("  gyro_y_raw="));
  out.println(gyro.yRaw);
  out.print(F("  gyro_z_raw="));
  out.println(gyro.zRaw);
  out.print(F("  gyro_x_mdeg_s="));
  out.println(gyro.xMdegPerSec);
  out.print(F("  gyro_y_mdeg_s="));
  out.println(gyro.yMdegPerSec);
  out.print(F("  gyro_z_mdeg_s="));
  out.println(gyro.zMdegPerSec);
  printImuRegisterDump(out);
}

void HardwareSelfTestService::printImuRegisterDump(Print& out) const {
  out.println(F("  registers_0x00_0x0f:"));
  for (uint8_t reg = 0x00; reg <= 0x0F; ++reg) {
    uint8_t value = 0xFF;
    out.print(F("    reg_0x"));
    printHexByte(out, reg);
    out.print(F("="));
    if (imu_.readDebugRegister(reg, value)) {
      out.print(F("0x"));
      printHexByte(out, value);
      out.println();
    } else {
      out.println(F("read_failed"));
    }
  }
}

void HardwareSelfTestService::printLogoTouch(Print& out) const {
  out.println(F("logo touch:"));
  printBool(out, F("  ready="), logoTouch_.ready());
  out.print(F("  raw="));
  out.println(logoTouch_.raw());
  out.print(F("  baseline="));
  out.println(logoTouch_.baseline());
  out.print(F("  delta="));
  out.println(logoTouch_.delta());
  out.print(F("  threshold="));
  out.println(logoTouch_.threshold());
  printBool(out, F("  touched="), logoTouched(logoTouch_));
}

void HardwareSelfTestService::watchLogoTouch(Print& out) {
  out.println(F("logo touch watch:"));
  out.println(F("  duration_ms=5000"));
  out.println(F("  sample_ms=100"));
  out.println(F("  instruction=touch and release the logo now"));
  const unsigned long startedMs = millis();
  unsigned long nextPrintMs = startedMs;
  while (millis() - startedMs < 5000UL) {
    logoTouch_.update();
    const unsigned long now = millis();
    if (static_cast<long>(now - nextPrintMs) >= 0) {
      out.print(F("  t_ms="));
      out.print(now - startedMs);
      out.print(F(" raw="));
      out.print(logoTouch_.raw());
      out.print(F(" baseline="));
      out.print(logoTouch_.baseline());
      out.print(F(" delta="));
      out.print(logoTouch_.delta());
      out.print(F(" threshold="));
      out.print(logoTouch_.threshold());
      out.print(F(" touched="));
      out.println(logoTouched(logoTouch_) ? F("1") : F("0"));
      nextPrintMs = now + 100UL;
    }
    delay(5);
  }
  printLogoTouch(out);
}

void HardwareSelfTestService::printImuRawTest(Print& out) {
  out.println(F("imu raw test:"));
  printBool(out, F("  ready="), imu_.ready());
  if (!imu_.ready()) {
    return;
  }

  out.print(F("  address=0x"));
  out.println(imu_.address(), HEX);
  out.print(F("  who=0x"));
  out.println(imu_.whoAmI(), HEX);
  printImuRegisterDump(out);
  out.print(F("  bias_ms="));
  out.println(kImuRawBiasMs);
  out.print(F("  motion_ms="));
  out.println(kImuRawMotionMs);
  out.print(F("  sample_ms="));
  out.println(kImuRawSampleMs);
  out.println(F("  instruction=keep still during bias, then rotate left or right by hand"));
  Serial.println(F("imu raw test begin"));

  int64_t biasRawSum[3] = {};
  int64_t biasDpsSum[3] = {};
  uint16_t biasSamples = 0;
  const unsigned long biasStarted = millis();
  while (millis() - biasStarted < kImuRawBiasMs) {
    const ImuGyroSnapshot gyro = imu_.readGyro();
    if (!gyro.ready) {
      out.println(F("  error=gyro_read_failed_during_bias"));
      Serial.println(F("imu raw test failed during bias"));
      return;
    }
    biasRawSum[0] += gyro.xRaw;
    biasRawSum[1] += gyro.yRaw;
    biasRawSum[2] += gyro.zRaw;
    biasDpsSum[0] += gyro.xMdegPerSec;
    biasDpsSum[1] += gyro.yMdegPerSec;
    biasDpsSum[2] += gyro.zMdegPerSec;
    ++biasSamples;
    delay(kImuRawSampleMs);
  }

  if (biasSamples == 0) {
    out.println(F("  error=no_bias_samples"));
    return;
  }

  int32_t biasRaw[3] = {};
  int32_t biasDps[3] = {};
  for (uint8_t axis = 0; axis < 3; ++axis) {
    biasRaw[axis] = static_cast<int32_t>(biasRawSum[axis] / biasSamples);
    biasDps[axis] = static_cast<int32_t>(biasDpsSum[axis] / biasSamples);
  }

  int16_t minRaw[3] = {32767, 32767, 32767};
  int16_t maxRaw[3] = {-32768, -32768, -32768};
  int16_t minAccelRaw[3] = {32767, 32767, 32767};
  int16_t maxAccelRaw[3] = {-32768, -32768, -32768};
  int16_t firstRaw[3] = {};
  int16_t lastRaw[3] = {};
  int16_t firstAccelRaw[3] = {};
  int16_t lastAccelRaw[3] = {};
  uint8_t firstStatus0 = 0;
  uint8_t lastStatus0 = 0;
  uint8_t firstStatus1 = 0;
  uint8_t lastStatus1 = 0;
  uint32_t firstTimestamp = 0;
  uint32_t lastTimestamp = 0;
  int16_t firstTempRaw = 0;
  int16_t lastTempRaw = 0;
  int32_t minDps[3] = {INT32_MAX, INT32_MAX, INT32_MAX};
  int32_t maxDps[3] = {INT32_MIN, INT32_MIN, INT32_MIN};
  int64_t angleMdeg[3] = {};
  int64_t absRateSum[3] = {};
  uint16_t motionSamples = 0;
  const unsigned long motionStarted = millis();
  while (millis() - motionStarted < kImuRawMotionMs) {
    ImuDebugFrame frame;
    if (!readImuDebugFrame(imu_, frame)) {
      out.println(F("  error=gyro_read_failed_during_motion"));
      Serial.println(F("imu raw test failed during motion"));
      return;
    }

    const int16_t raw[3] = {frame.gyroRaw[0], frame.gyroRaw[1],
                            frame.gyroRaw[2]};
    const int16_t accelRaw[3] = {frame.accelRaw[0], frame.accelRaw[1],
                                 frame.accelRaw[2]};
    const int32_t dps[3] = {
        static_cast<int32_t>(raw[0]) * 1000L / kImuGyroLsbPerDps,
        static_cast<int32_t>(raw[1]) * 1000L / kImuGyroLsbPerDps,
        static_cast<int32_t>(raw[2]) * 1000L / kImuGyroLsbPerDps};
    if (motionSamples == 0) {
      firstStatus0 = frame.status0;
      firstStatus1 = frame.status1;
      firstTimestamp = frame.timestamp;
      firstTempRaw = frame.tempRaw;
      firstRaw[0] = raw[0];
      firstRaw[1] = raw[1];
      firstRaw[2] = raw[2];
      firstAccelRaw[0] = accelRaw[0];
      firstAccelRaw[1] = accelRaw[1];
      firstAccelRaw[2] = accelRaw[2];
    }
    lastStatus0 = frame.status0;
    lastStatus1 = frame.status1;
    lastTimestamp = frame.timestamp;
    lastTempRaw = frame.tempRaw;
    lastRaw[0] = raw[0];
    lastRaw[1] = raw[1];
    lastRaw[2] = raw[2];
    lastAccelRaw[0] = accelRaw[0];
    lastAccelRaw[1] = accelRaw[1];
    lastAccelRaw[2] = accelRaw[2];
    for (uint8_t axis = 0; axis < 3; ++axis) {
      minRaw[axis] = min(minRaw[axis], raw[axis]);
      maxRaw[axis] = max(maxRaw[axis], raw[axis]);
      minAccelRaw[axis] = min(minAccelRaw[axis], accelRaw[axis]);
      maxAccelRaw[axis] = max(maxAccelRaw[axis], accelRaw[axis]);
      minDps[axis] = min(minDps[axis], dps[axis]);
      maxDps[axis] = max(maxDps[axis], dps[axis]);
      const int32_t rate = dps[axis] - biasDps[axis];
      angleMdeg[axis] +=
          static_cast<int64_t>(rate) * static_cast<int64_t>(kImuRawSampleMs) /
          1000;
      absRateSum[axis] += abs(rate);
    }
    ++motionSamples;
    delay(kImuRawSampleMs);
  }

  uint8_t maxAxis = 0;
  for (uint8_t axis = 1; axis < 3; ++axis) {
    if (absRateSum[axis] > absRateSum[maxAxis]) {
      maxAxis = axis;
    }
  }

  out.print(F("  bias_samples="));
  out.println(biasSamples);
  out.print(F("  motion_samples="));
  out.println(motionSamples);
  out.print(F("  bias_x_raw="));
  out.println(biasRaw[0]);
  out.print(F("  bias_y_raw="));
  out.println(biasRaw[1]);
  out.print(F("  bias_z_raw="));
  out.println(biasRaw[2]);
  out.print(F("  bias_x_mdeg_s="));
  out.println(biasDps[0]);
  out.print(F("  bias_y_mdeg_s="));
  out.println(biasDps[1]);
  out.print(F("  bias_z_mdeg_s="));
  out.println(biasDps[2]);
  out.print(F("  first_status0=0x"));
  printHexByte(out, firstStatus0);
  out.println();
  out.print(F("  last_status0=0x"));
  printHexByte(out, lastStatus0);
  out.println();
  out.print(F("  first_status1=0x"));
  printHexByte(out, firstStatus1);
  out.println();
  out.print(F("  last_status1=0x"));
  printHexByte(out, lastStatus1);
  out.println();
  out.print(F("  first_timestamp="));
  out.println(firstTimestamp);
  out.print(F("  last_timestamp="));
  out.println(lastTimestamp);
  out.print(F("  first_temp_raw="));
  out.println(firstTempRaw);
  out.print(F("  last_temp_raw="));
  out.println(lastTempRaw);
  out.print(F("  first_ax_raw="));
  out.println(firstAccelRaw[0]);
  out.print(F("  first_ay_raw="));
  out.println(firstAccelRaw[1]);
  out.print(F("  first_az_raw="));
  out.println(firstAccelRaw[2]);
  out.print(F("  last_ax_raw="));
  out.println(lastAccelRaw[0]);
  out.print(F("  last_ay_raw="));
  out.println(lastAccelRaw[1]);
  out.print(F("  last_az_raw="));
  out.println(lastAccelRaw[2]);
  out.print(F("  first_x_raw="));
  out.println(firstRaw[0]);
  out.print(F("  first_y_raw="));
  out.println(firstRaw[1]);
  out.print(F("  first_z_raw="));
  out.println(firstRaw[2]);
  out.print(F("  last_x_raw="));
  out.println(lastRaw[0]);
  out.print(F("  last_y_raw="));
  out.println(lastRaw[1]);
  out.print(F("  last_z_raw="));
  out.println(lastRaw[2]);
  for (uint8_t axis = 0; axis < 3; ++axis) {
    out.print(F("  gyro_"));
    out.print(axisName(axis));
    out.print(F("_raw_min="));
    out.println(minRaw[axis]);
    out.print(F("  gyro_"));
    out.print(axisName(axis));
    out.print(F("_raw_max="));
    out.println(maxRaw[axis]);
    out.print(F("  gyro_"));
    out.print(axisName(axis));
    out.print(F("_mdeg_s_min="));
    out.println(minDps[axis]);
    out.print(F("  gyro_"));
    out.print(axisName(axis));
    out.print(F("_mdeg_s_max="));
    out.println(maxDps[axis]);
    out.print(F("  angle_"));
    out.print(axisName(axis));
    out.print(F("_mdeg="));
    out.println(static_cast<long>(angleMdeg[axis]));
    out.print(F("  accel_"));
    out.print(axisName(axis));
    out.print(F("_raw_min="));
    out.println(minAccelRaw[axis]);
    out.print(F("  accel_"));
    out.print(axisName(axis));
    out.print(F("_raw_max="));
    out.println(maxAccelRaw[axis]);
  }
  out.print(F("  max_axis="));
  out.println(axisName(maxAxis));
  out.print(F("  axis_sign="));
  out.println(signOf(angleMdeg[maxAxis]));

  Serial.print(F("imu raw test done max_axis="));
  Serial.print(axisName(maxAxis));
  Serial.print(F(" sign="));
  Serial.print(signOf(angleMdeg[maxAxis]));
  Serial.print(F(" angle_x="));
  Serial.print(static_cast<long>(angleMdeg[0]));
  Serial.print(F(" angle_y="));
  Serial.print(static_cast<long>(angleMdeg[1]));
  Serial.print(F(" angle_z="));
  Serial.println(static_cast<long>(angleMdeg[2]));
}

void HardwareSelfTestService::testSpeaker(Print& out) {
  audioOutput_.playTestTone(kSpeakerTestHz, kSpeakerTestMs);
  out.println(F("speaker test tone done"));
}

void HardwareSelfTestService::printAudioVolume(Print& out) const {
  out.println(F("audio:"));
  out.print(F("  volume_percent="));
  out.println(audioOutput_.volumePercent());
}

void HardwareSelfTestService::startShippingDischarge(Print& out) {
  const BatterySnapshot snapshot = battery_.read();
  shippingDischargeDone_ = false;
  shippingDischargeFailed_ = false;

  if (snapshot.usbPresent) {
    finishShippingDischarge(true, "usb present, unplug USB before discharge",
                            false);
    printShippingDischargeStatus(out);
    return;
  }

  if (snapshot.percent <= kShippingDischargeTargetPercent) {
    finishShippingDischarge(false, "already at or below target", true);
    printShippingDischargeStatus(out);
    return;
  }

  stopMotorPulse();
  cancelAutoMotorCalibration();
  motorCalibrationPhase_ = 0;
  manualMotorTimeoutMs_ = 0;
  motorStopAtMs_ = 0;

  shippingDischargeActive_ = true;
  shippingDischargeMotorOn_ = false;
  shippingDischargeStartedMs_ = millis();
  shippingDischargePhaseStartedMs_ = 0;
  snprintf(shippingDischargeMessage_, sizeof(shippingDischargeMessage_),
           "running");
  updateShippingDischarge();
  printShippingDischargeStatus(out);
}

void HardwareSelfTestService::stopShippingDischarge(Print& out) {
  if (shippingDischargeActive_) {
    finishShippingDischarge(false, "stopped by user", false);
  }
  printShippingDischargeStatus(out);
}

void HardwareSelfTestService::printShippingDischargeStatus(Print& out) const {
  const BatterySnapshot snapshot = battery_.read();
  out.println(F("shipping discharge:"));
  printBool(out, F("  active="), shippingDischargeActive_);
  printBool(out, F("  done="), shippingDischargeDone_);
  printBool(out, F("  failed="), shippingDischargeFailed_);
  printBool(out, F("  motor_on="), shippingDischargeMotorOn_);
  out.print(F("  message="));
  out.println(shippingDischargeMessage_);
  out.print(F("  target_percent="));
  out.println(kShippingDischargeTargetPercent);
  out.print(F("  current_percent="));
  out.println(snapshot.percent);
  out.print(F("  voltage_mv="));
  out.println(snapshot.voltageMv);
  printBool(out, F("  usb="), snapshot.usbPresent);
  out.print(F("  max_minutes="));
  out.println(kShippingDischargeMaxMs / 60000UL);
  out.println(F("  note=estimated_soc_for_board_test_only"));
}

void HardwareSelfTestService::testLed(const RgbColor& color, Print& out) {
  led_.show(color);
  out.println(F("led test color set"));
}

void HardwareSelfTestService::pulseLedDataPin(Print& out) {
  led_.off();
  pinMode(pins::LED_DATA, OUTPUT);
  out.println(F("led data pin pulse start"));
  out.print(F("  pin=IO"));
  out.println(static_cast<int>(pins::LED_DATA));
  for (uint8_t index = 0; index < 6; ++index) {
    digitalWrite(pins::LED_DATA, HIGH);
    delay(250);
    digitalWrite(pins::LED_DATA, LOW);
    delay(250);
  }
  out.println(F("led data pin pulse done"));
}

void HardwareSelfTestService::testMotor(WheelDrive left, WheelDrive right, Print& out) {
  if (shippingDischargeActive_) {
    finishShippingDischarge(false, "interrupted by motor pulse", false);
  }
  manualMotorTimeoutMs_ = 0;
  motors_.drive(left, right);
  motorStopAtMs_ = millis() + kMotorPulseMs;
  out.println(F("motor pulse started"));
}

void HardwareSelfTestService::printMotorDriverDiagnostic(const __FlashStringHelper* step,
                                                         Print& out) const {
  const MotorDriverDiagnostic snapshot = motors_.diagnostic();
  const auto printPin = [&out](const MotorPinDiagnostic& pin) {
    out.print(F("  "));
    out.print(pin.name);
    out.print(F(": gpio=IO"));
    out.print(pin.gpio);
    out.print(F(" logic="));
    out.print(pin.logicLevel == HIGH ? F("HIGH") : F("LOW"));
    out.print(F(" pwm_channel="));
    out.print(pin.pwmChannel);
    out.print(F(" duty="));
    out.println(pin.duty);
  };

  out.println(F("motor driver diagnostic:"));
  out.print(F("  step="));
  out.println(step);
  out.print(F("  sleep_gpio=IO"));
  out.println(static_cast<int>(pins::MOTOR_SLEEP));
  out.print(F("  sleep_logic="));
  out.println(digitalRead(pins::MOTOR_SLEEP) == HIGH ? F("HIGH") : F("LOW"));
  out.print(F("  fault_gpio=IO"));
  out.println(static_cast<int>(pins::MOTOR_FAULT));
  out.print(F("  fault_logic="));
  out.println(digitalRead(pins::MOTOR_FAULT) == HIGH ? F("HIGH") : F("LOW"));
  printBool(out, F("  driver_awake="), snapshot.awake);
  printBool(out, F("  left_inverted="), motors_.leftInverted());
  printBool(out, F("  right_inverted="), motors_.rightInverted());
  out.print(F("  left_command="));
  out.println(wheelDriveName(snapshot.leftCommand));
  out.print(F("  right_command="));
  out.println(wheelDriveName(snapshot.rightCommand));
  out.print(F("  left_applied="));
  out.println(wheelDriveName(snapshot.leftApplied));
  out.print(F("  right_applied="));
  out.println(wheelDriveName(snapshot.rightApplied));
  out.print(F("  left_duty="));
  out.println(snapshot.leftDuty);
  out.print(F("  right_duty="));
  out.println(snapshot.rightDuty);
  printPin(snapshot.ain1);
  printPin(snapshot.ain2);
  printPin(snapshot.bin1);
  printPin(snapshot.bin2);
}

void HardwareSelfTestService::runMotorDriverDiagnostic(Print& out, bool testA,
                                                       bool testB) {
  if (shippingDischargeActive_) {
    finishShippingDischarge(false, "interrupted by motor diagnostic", false);
  }
  cancelAutoMotorCalibration();
  motorCalibrationPhase_ = 0;
  motorCalibrationNextMs_ = 0;
  motorStopAtMs_ = 0;
  manualMotorTimeoutMs_ = 0;

  out.println(F("motor driver diagnostic begin"));
  out.print(F("  pwm="));
  out.println(kMotorDriverDiagnosticPwm);
  out.print(F("  step_ms="));
  out.println(kMotorDriverDiagnosticStepMs);
  out.print(F("  sequence="));
  if (testA && testB) {
    out.println(F("A_forward,A_reverse,B_forward,B_reverse,stop"));
  } else if (testA) {
    out.println(F("A_forward,A_reverse,stop"));
  } else {
    out.println(F("B_forward,B_reverse,stop"));
  }

  if (testA) {
    motors_.drive(WheelDrive::Forward, WheelDrive::Stop, 0,
                  kMotorDriverDiagnosticPwm, kMotorDriverDiagnosticPwm);
    printMotorDriverDiagnostic(F("A_forward"), out);
    delay(kMotorDriverDiagnosticStepMs);

    motors_.drive(WheelDrive::Reverse, WheelDrive::Stop, 0,
                  kMotorDriverDiagnosticPwm, kMotorDriverDiagnosticPwm);
    printMotorDriverDiagnostic(F("A_reverse"), out);
    delay(kMotorDriverDiagnosticStepMs);
  }

  if (testB) {
    motors_.drive(WheelDrive::Stop, WheelDrive::Forward, 0,
                  kMotorDriverDiagnosticPwm, kMotorDriverDiagnosticPwm);
    printMotorDriverDiagnostic(F("B_forward"), out);
    delay(kMotorDriverDiagnosticStepMs);

    motors_.drive(WheelDrive::Stop, WheelDrive::Reverse, 0,
                  kMotorDriverDiagnosticPwm, kMotorDriverDiagnosticPwm);
    printMotorDriverDiagnostic(F("B_reverse"), out);
    delay(kMotorDriverDiagnosticStepMs);
  }

  motors_.stop();
  printMotorDriverDiagnostic(F("stop"), out);
  out.println(F("motor driver diagnostic done"));
}

void HardwareSelfTestService::startManualMotor(WheelDrive direction,
                                               const String& command,
                                               Print& out) {
  if (shippingDischargeActive_) {
    finishShippingDischarge(false, "interrupted by manual motor", false);
  }
  motorStopAtMs_ = 0;

  uint8_t leftPwm = motors_.leftDefaultDuty();
  uint8_t rightPwm = motors_.rightDefaultDuty();
  int parsed = 0;
  int start = command.indexOf(' ', command.indexOf(F("manual")));
  if (start >= 0) {
    start = command.indexOf(' ', start + 1);
  }
  while (start >= 0 && parsed < 2) {
    const int next = command.indexOf(' ', start + 1);
    const String token = command.substring(start + 1, next < 0 ? command.length() : next);
    if (token.length() > 0) {
      const int value = token.toInt();
      if (value > 0 && value <= 255) {
        if (parsed == 0) {
          leftPwm = static_cast<uint8_t>(value);
        } else {
          rightPwm = static_cast<uint8_t>(value);
        }
        ++parsed;
      }
    }
    start = next;
  }

  motors_.drive(direction, direction, 0, leftPwm, rightPwm);
  manualMotorTimeoutMs_ = millis() + kManualMotorTimeoutMs;

  out.println(F("motor manual keepalive"));
  out.print(F("  direction="));
  out.println(direction == WheelDrive::Forward ? F("forward") : F("reverse"));
  out.print(F("  left_pwm="));
  out.println(leftPwm);
  out.print(F("  right_pwm="));
  out.println(rightPwm);
  out.print(F("  timeout_ms="));
  out.println(kManualMotorTimeoutMs);
}

void HardwareSelfTestService::startAutoMotorCalibration(WheelDrive direction,
                                                        const String& command,
                                                        Print& out) {
  if (shippingDischargeActive_) {
    finishShippingDischarge(false, "interrupted by gyro straight", false);
  }
  if (!imu_.ready()) {
    autoMotorFailed_ = true;
    autoMotorDone_ = true;
    autoMotorPhase_ = 0;
    snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "imu not ready");
    out.println(F("motor auto failed"));
    out.println(F("  reason=imu_not_ready"));
    return;
  }

  uint8_t basePwm = kMotorAutoBasePwm;
  int start = command.indexOf(' ', command.indexOf(F("auto")));
  if (start >= 0) {
    start = command.indexOf(' ', start + 1);
  }
  while (start >= 0) {
    const int next = command.indexOf(' ', start + 1);
    const String token = command.substring(start + 1, next < 0 ? command.length() : next);
    if (token.length() > 0) {
      const int value = token.toInt();
      if (value >= kMotorAutoMinPwm && value <= kMotorAutoMaxPwm) {
        basePwm = static_cast<uint8_t>(value);
        break;
      }
    }
    start = next;
  }

  stopMotorPulse();
  autoMotorDirection_ = direction;
  autoMotorLeftPwm_ = basePwm;
  autoMotorRightPwm_ = basePwm;
  autoMotorNewLeftPwm_ = basePwm;
  autoMotorNewRightPwm_ = basePwm;
  resetAutoMotorSamples();
  autoMotorDone_ = false;
  autoMotorFailed_ = false;
  autoMotorAxis_ = -1;
  autoMotorPhase_ = 1;
  autoMotorPhaseStartedMs_ = millis();
  autoMotorNextMs_ = millis();
  autoMotorControlNextMs_ = 0;
  autoMotorLastSampleMs_ = 0;
  autoMotorCorrection_ = 0;
  autoMotorLiveCorrection_ = 0;
  autoMotorBiasTerm_ = 0;
  snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro bias sampling");

  out.println(F("motor gyro straight begin"));
  out.print(F("  direction="));
  out.println(direction == WheelDrive::Forward ? F("forward") : F("reverse"));
  out.print(F("  base_pwm="));
  out.println(basePwm);
  out.println(F("  note=keep TongDou on a clear flat table"));
  Serial.print(F("motor gyro straight begin direction="));
  Serial.print(direction == WheelDrive::Forward ? F("forward") : F("reverse"));
  Serial.print(F(" base_pwm="));
  Serial.print(basePwm);
  Serial.print(F(" saved_left_pwm="));
  Serial.print(motors_.leftDefaultDuty());
  Serial.print(F(" saved_right_pwm="));
  Serial.print(motors_.rightDefaultDuty());
  Serial.println(F(" note=auto_uses_base_pwm_not_saved_trim"));
}

void HardwareSelfTestService::printAutoMotorCalibration(Print& out) const {
  out.println(F("motor auto:"));
  printBool(out, F("  active="), autoMotorPhase_ != 0);
  printBool(out, F("  done="), autoMotorDone_);
  printBool(out, F("  failed="), autoMotorFailed_);
  out.print(F("  phase="));
  out.println(autoMotorPhase_);
  out.print(F("  message="));
  out.println(autoMotorMessage_);
  out.print(F("  axis="));
  out.println(autoMotorAxis_);
  out.print(F("  old_left_pwm="));
  out.println(autoMotorLeftPwm_);
  out.print(F("  old_right_pwm="));
  out.println(autoMotorRightPwm_);
  out.print(F("  new_left_pwm="));
  out.println(autoMotorNewLeftPwm_);
  out.print(F("  new_right_pwm="));
  out.println(autoMotorNewRightPwm_);
  out.print(F("  correction="));
  out.println(autoMotorCorrection_);
  out.print(F("  live_correction="));
  out.println(autoMotorLiveCorrection_);
  out.print(F("  bias_term="));
  out.println(autoMotorBiasTerm_);
  out.print(F("  last_yaw_rate_mdeg_s="));
  out.println(autoMotorLastYawRate_);
  out.print(F("  last_aligned_rate_mdeg_s="));
  out.println(autoMotorLastAlignedRate_);
  out.print(F("  last_aligned_angle_mdeg="));
  out.println(autoMotorLastAlignedAngle_);
  out.print(F("  rate_correction="));
  out.println(autoMotorLastRateCorrection_);
  out.print(F("  heading_correction="));
  out.println(autoMotorLastHeadingCorrection_);
  out.print(F("  bias_samples="));
  out.println(autoMotorBiasSamples_);
  out.print(F("  straight_samples="));
  out.println(autoMotorStraightSamples_);
  out.print(F("  probe_angle_x_mdeg="));
  out.println(static_cast<long>(autoMotorProbeAngleX_));
  out.print(F("  probe_angle_y_mdeg="));
  out.println(static_cast<long>(autoMotorProbeAngleY_));
  out.print(F("  probe_angle_z_mdeg="));
  out.println(static_cast<long>(autoMotorProbeAngleZ_));
  out.print(F("  straight_angle_x_mdeg="));
  out.println(static_cast<long>(autoMotorStraightAngleX_));
  out.print(F("  straight_angle_y_mdeg="));
  out.println(static_cast<long>(autoMotorStraightAngleY_));
  out.print(F("  straight_angle_z_mdeg="));
  out.println(static_cast<long>(autoMotorStraightAngleZ_));
}

void HardwareSelfTestService::updateAutoMotorCalibration() {
  if (autoMotorPhase_ == 0) {
    return;
  }

  const unsigned long now = millis();
  if (autoMotorNextMs_ != 0 && static_cast<long>(now - autoMotorNextMs_) < 0) {
    return;
  }

  if (autoMotorPhase_ == 1) {
    const ImuGyroSnapshot gyro = imu_.readGyro();
    if (!gyro.ready) {
      autoMotorFailed_ = true;
      autoMotorDone_ = true;
      autoMotorPhase_ = 0;
      snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro read failed");
      motors_.stop();
      return;
    }

    autoMotorBiasSumX_ += gyro.xMdegPerSec;
    autoMotorBiasSumY_ += gyro.yMdegPerSec;
    autoMotorBiasSumZ_ += gyro.zMdegPerSec;
    ++autoMotorBiasSamples_;
    autoMotorNextMs_ = now + kMotorAutoSampleMs;

    if (now - autoMotorPhaseStartedMs_ < kMotorAutoBiasMs) {
      return;
    }

    if (autoMotorBiasSamples_ == 0) {
      autoMotorFailed_ = true;
      autoMotorDone_ = true;
      autoMotorPhase_ = 0;
      snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "no bias samples");
      return;
    }

    autoMotorBiasX_ = static_cast<int32_t>(autoMotorBiasSumX_ / autoMotorBiasSamples_);
    autoMotorBiasY_ = static_cast<int32_t>(autoMotorBiasSumY_ / autoMotorBiasSamples_);
    autoMotorBiasZ_ = static_cast<int32_t>(autoMotorBiasSumZ_ / autoMotorBiasSamples_);
    Serial.print(F("motor gyro bias x="));
    Serial.print(autoMotorBiasX_);
    Serial.print(F(" y="));
    Serial.print(autoMotorBiasY_);
    Serial.print(F(" z="));
    Serial.print(autoMotorBiasZ_);
    Serial.print(F(" samples="));
    Serial.println(autoMotorBiasSamples_);
    autoMotorPhase_ = 2;
    autoMotorPhaseStartedMs_ = now;
    autoMotorNextMs_ = now + kMotorAutoSoftStartMs;
    autoMotorStraightAngleX_ = 0;
    autoMotorStraightAngleY_ = 0;
    autoMotorStraightAngleZ_ = 0;
    autoMotorStraightSamples_ = 0;
    autoMotorCorrection_ = 0;
    autoMotorLiveCorrection_ = 0;
    autoMotorBiasTerm_ = 0;
    autoMotorNewLeftPwm_ = clampAutoPwm(static_cast<int>(autoMotorLeftPwm_) -
                                        kMotorAutoSoftStartPwmDrop);
    autoMotorNewRightPwm_ = autoMotorNewLeftPwm_;
    autoMotorControlNextMs_ = 0;
    autoMotorLastSampleMs_ = 0;
    snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro soft start");
    autoMotorProbeAngleZ_ = kMotorAutoForwardProbeSign;
    Serial.print(F("motor gyro soft start ms="));
    Serial.print(kMotorAutoSoftStartMs);
    Serial.print(F(" pwm="));
    Serial.println(autoMotorNewLeftPwm_);
    printAutoMotorOutput(F("first_drive"), autoMotorNewLeftPwm_,
                         autoMotorNewRightPwm_, autoMotorCorrection_);
    motors_.drive(autoMotorDirection_, autoMotorDirection_, 0,
                  autoMotorNewLeftPwm_, autoMotorNewRightPwm_);
    return;
  }

  if (autoMotorPhase_ == 2) {
    autoMotorPhase_ = 4;
    autoMotorPhaseStartedMs_ = now;
    autoMotorNextMs_ = now;
    autoMotorControlNextMs_ = now;
    autoMotorStraightAngleX_ = 0;
    autoMotorStraightAngleY_ = 0;
    autoMotorStraightAngleZ_ = 0;
    autoMotorStraightSamples_ = 0;
    autoMotorCorrection_ = 0;
    autoMotorLiveCorrection_ = 0;
    autoMotorBiasTerm_ = 0;
    autoMotorLastSampleMs_ = 0;
    autoMotorLastYawRate_ = 0;
    autoMotorLastAlignedRate_ = 0;
    autoMotorLastAlignedAngle_ = 0;
    autoMotorLastRateCorrection_ = 0;
    autoMotorLastHeadingCorrection_ = 0;
    snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro straight correcting");
    Serial.println(F("motor gyro closed loop start reset_angle=1 ramp_from_zero=1"));
    return;
  }

  if (autoMotorPhase_ == 4) {
    const ImuGyroSnapshot gyro = imu_.readGyro();
    if (!gyro.ready) {
      autoMotorFailed_ = true;
      autoMotorDone_ = true;
      autoMotorPhase_ = 0;
      snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro read failed");
      motors_.stop();
      return;
    }

    const int32_t xRate = gyro.xMdegPerSec - autoMotorBiasX_;
    const int32_t yRate = gyro.yMdegPerSec - autoMotorBiasY_;
    const int32_t yawRate = gyro.zMdegPerSec - autoMotorBiasZ_;
    autoMotorLastYawRate_ = yawRate;

    unsigned long sampleDtMs = kMotorAutoSampleMs;
    if (autoMotorLastSampleMs_ != 0) {
      sampleDtMs = now - autoMotorLastSampleMs_;
      sampleDtMs = constrain(sampleDtMs, 1UL, 80UL);
    }
    autoMotorLastSampleMs_ = now;

    autoMotorStraightAngleX_ +=
        static_cast<int64_t>(xRate) * static_cast<int64_t>(sampleDtMs) / 1000;
    autoMotorStraightAngleY_ +=
        static_cast<int64_t>(yRate) * static_cast<int64_t>(sampleDtMs) / 1000;
    autoMotorStraightAngleZ_ +=
        static_cast<int64_t>(yawRate) * static_cast<int64_t>(sampleDtMs) / 1000;
    ++autoMotorStraightSamples_;

    const int32_t alignedRate = yawRate * kMotorAutoForwardProbeSign;
    const int32_t alignedAngle =
        static_cast<int32_t>(autoMotorStraightAngleZ_ * kMotorAutoForwardProbeSign);
    autoMotorLastAlignedRate_ = alignedRate;
    autoMotorLastAlignedAngle_ = alignedAngle;

    if (static_cast<long>(now - autoMotorControlNextMs_) >= 0) {
      const int rateCorrection = signedScaledCorrection(
          alignedRate, kMotorAutoRateDeadband, kMotorAutoRateCorrectionDivisor,
          kMotorAutoMaxLiveCorrection);
      const int headingCorrection = signedScaledCorrection(
          alignedAngle, kMotorAutoHeadingDeadband,
          kMotorAutoHeadingCorrectionDivisor, kMotorAutoMaxHeadingCorrection);
      const int targetLiveCorrection = constrain(
          rateCorrection + headingCorrection,
          -static_cast<int>(kMotorAutoMaxLiveCorrection),
          static_cast<int>(kMotorAutoMaxLiveCorrection));
      const int targetCorrection = constrain(
          targetLiveCorrection,
          -static_cast<int>(kMotorAutoMaxCorrection),
          static_cast<int>(kMotorAutoMaxCorrection));
      autoMotorLastRateCorrection_ = static_cast<int16_t>(rateCorrection);
      autoMotorLastHeadingCorrection_ = static_cast<int16_t>(headingCorrection);
      autoMotorBiasTerm_ = 0;
      autoMotorCorrection_ = stepTowardCorrection(
          autoMotorCorrection_, static_cast<int16_t>(targetCorrection));
      autoMotorLiveCorrection_ = autoMotorCorrection_;
      autoMotorNewLeftPwm_ =
          clampAutoPwm(static_cast<int>(autoMotorLeftPwm_) - autoMotorCorrection_);
      autoMotorNewRightPwm_ =
          clampAutoPwm(static_cast<int>(autoMotorRightPwm_) + autoMotorCorrection_);
      motors_.drive(autoMotorDirection_, autoMotorDirection_, 0,
                    autoMotorNewLeftPwm_, autoMotorNewRightPwm_);
      autoMotorControlNextMs_ = now + kMotorAutoControlMs;

      Serial.print(F("motor gyro step yaw_rate="));
      Serial.print(yawRate);
      Serial.print(F(" aligned_rate="));
      Serial.print(alignedRate);
      Serial.print(F(" aligned_angle="));
      Serial.print(alignedAngle);
      Serial.print(F(" angle_x="));
      Serial.print(static_cast<long>(autoMotorStraightAngleX_));
      Serial.print(F(" angle_y="));
      Serial.print(static_cast<long>(autoMotorStraightAngleY_));
      Serial.print(F(" angle_z="));
      Serial.print(static_cast<long>(autoMotorStraightAngleZ_));
      Serial.print(F(" dt_ms="));
      Serial.print(sampleDtMs);
      Serial.print(F(" target="));
      Serial.print(targetCorrection);
      Serial.print(F(" bias_term="));
      Serial.print(autoMotorBiasTerm_);
      Serial.print(F(" target_live="));
      Serial.print(targetLiveCorrection);
      Serial.print(F(" rate_term="));
      Serial.print(rateCorrection);
      Serial.print(F(" heading_term="));
      Serial.print(headingCorrection);
      Serial.print(F(" correction="));
      Serial.print(autoMotorCorrection_);
      Serial.print(F(" live="));
      Serial.print(autoMotorLiveCorrection_);
      Serial.print(F(" left_pwm="));
      Serial.print(autoMotorNewLeftPwm_);
      Serial.print(F(" right_pwm="));
      Serial.println(autoMotorNewRightPwm_);
      printAutoMotorOutput(F("control_step"), autoMotorNewLeftPwm_,
                           autoMotorNewRightPwm_, autoMotorCorrection_);
    }

    autoMotorNextMs_ = now + kMotorAutoSampleMs;
    if (now - autoMotorPhaseStartedMs_ < kMotorAutoStraightMs) {
      return;
    }
    motors_.stop();
    printAutoMotorOutput(F("soft_stop"), 0, 0, 0);
    autoMotorPhase_ = 5;
    autoMotorPhaseStartedMs_ = now;
    autoMotorNextMs_ = now + kMotorAutoBrakeMs;
    snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "straight soft stop");
    return;
  }

  finishAutoMotorCalibration();
}

void HardwareSelfTestService::finishAutoMotorCalibration() {
  motors_.stop();
  printAutoMotorOutput(F("final_stop"), 0, 0, 0);

  autoMotorAxis_ = 2;

  autoMotorFailed_ = false;
  autoMotorDone_ = true;
  autoMotorPhase_ = 0;
  snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro straight done");
  Serial.print(F("motor gyro straight done old_left="));
  Serial.print(autoMotorLeftPwm_);
  Serial.print(F(" old_right="));
  Serial.print(autoMotorRightPwm_);
  Serial.print(F(" new_left="));
  Serial.print(autoMotorNewLeftPwm_);
  Serial.print(F(" new_right="));
  Serial.print(autoMotorNewRightPwm_);
  Serial.print(F(" probe_z="));
  Serial.print(static_cast<long>(autoMotorProbeAngleZ_));
  Serial.print(F(" straight_z="));
  Serial.print(static_cast<long>(autoMotorStraightAngleZ_));
  Serial.print(F(" samples="));
  Serial.println(autoMotorStraightSamples_);
}

void HardwareSelfTestService::cancelAutoMotorCalibration() {
  if (autoMotorPhase_ == 0) {
    return;
  }
  autoMotorPhase_ = 0;
  autoMotorDone_ = true;
  autoMotorFailed_ = true;
  snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "cancelled");
}

void HardwareSelfTestService::sampleAutoMotorGyro(bool probePhase) {
  const ImuGyroSnapshot gyro = imu_.readGyro();
  if (!gyro.ready) {
    autoMotorFailed_ = true;
    autoMotorDone_ = true;
    autoMotorPhase_ = 0;
    snprintf(autoMotorMessage_, sizeof(autoMotorMessage_), "gyro read failed");
    motors_.stop();
    Serial.println(F("motor gyro straight failed message=gyro read failed"));
    return;
  }

  const int32_t x = gyro.xMdegPerSec - autoMotorBiasX_;
  const int32_t y = gyro.yMdegPerSec - autoMotorBiasY_;
  const int32_t z = gyro.zMdegPerSec - autoMotorBiasZ_;
  if (probePhase) {
    autoMotorProbeAngleX_ += x * static_cast<int32_t>(kMotorAutoSampleMs) / 1000;
    autoMotorProbeAngleY_ += y * static_cast<int32_t>(kMotorAutoSampleMs) / 1000;
    autoMotorProbeAngleZ_ += z * static_cast<int32_t>(kMotorAutoSampleMs) / 1000;
    ++autoMotorProbeSamples_;
  } else {
    autoMotorStraightAngleX_ += x * static_cast<int32_t>(kMotorAutoSampleMs) / 1000;
    autoMotorStraightAngleY_ += y * static_cast<int32_t>(kMotorAutoSampleMs) / 1000;
    autoMotorStraightAngleZ_ += z * static_cast<int32_t>(kMotorAutoSampleMs) / 1000;
    ++autoMotorStraightSamples_;
  }
}

void HardwareSelfTestService::resetAutoMotorSamples() {
  autoMotorBiasX_ = 0;
  autoMotorBiasY_ = 0;
  autoMotorBiasZ_ = 0;
  autoMotorBiasSumX_ = 0;
  autoMotorBiasSumY_ = 0;
  autoMotorBiasSumZ_ = 0;
  autoMotorBiasSamples_ = 0;
  autoMotorProbeAngleX_ = 0;
  autoMotorProbeAngleY_ = 0;
  autoMotorProbeAngleZ_ = 0;
  autoMotorStraightAngleX_ = 0;
  autoMotorStraightAngleY_ = 0;
  autoMotorStraightAngleZ_ = 0;
  autoMotorProbeSamples_ = 0;
  autoMotorStraightSamples_ = 0;
  autoMotorCorrection_ = 0;
  autoMotorLiveCorrection_ = 0;
  autoMotorLastYawRate_ = 0;
  autoMotorLastAlignedRate_ = 0;
  autoMotorLastAlignedAngle_ = 0;
  autoMotorLastRateCorrection_ = 0;
  autoMotorLastHeadingCorrection_ = 0;
  autoMotorControlNextMs_ = 0;
}

void HardwareSelfTestService::startMotorCalibration(WheelDrive direction,
                                                    const String& command,
                                                    Print& out) {
  if (shippingDischargeActive_) {
    finishShippingDischarge(false, "interrupted by motor calibration", false);
  }
  motorStopAtMs_ = 0;
  manualMotorTimeoutMs_ = 0;

  uint8_t values[4] = {210, 210, 150, 150};
  int parsed = 0;
  int start = command.indexOf(' ', command.indexOf(F("cal")));
  if (start >= 0) {
    start = command.indexOf(' ', start + 1);
  }
  while (start >= 0 && parsed < 4) {
    const int next = command.indexOf(' ', start + 1);
    const String token = command.substring(start + 1, next < 0 ? command.length() : next);
    if (token.length() > 0) {
      const int value = token.toInt();
      if (value > 0 && value <= 255) {
        values[parsed++] = static_cast<uint8_t>(value);
      }
    }
    start = next;
  }

  motorCalibrationDirection_ = direction;
  motorCalibrationLeftStartPwm_ = values[0];
  motorCalibrationRightStartPwm_ = values[1];
  motorCalibrationLeftHoldPwm_ = values[2];
  motorCalibrationRightHoldPwm_ = values[3];
  motorCalibrationPhase_ = 1;
  motorCalibrationNextMs_ = millis() + kMotorCalibrationStartMs;

  motors_.drive(direction, direction, 0, motorCalibrationLeftStartPwm_,
                motorCalibrationRightStartPwm_);

  out.println(F("motor calibration begin"));
  out.print(F("  direction="));
  out.println(direction == WheelDrive::Forward ? F("forward") : F("reverse"));
  out.print(F("  start_left_pwm="));
  out.println(motorCalibrationLeftStartPwm_);
  out.print(F("  start_right_pwm="));
  out.println(motorCalibrationRightStartPwm_);
  out.print(F("  start_ms="));
  out.println(kMotorCalibrationStartMs);
  out.print(F("  hold_left_pwm="));
  out.println(motorCalibrationLeftHoldPwm_);
  out.print(F("  hold_right_pwm="));
  out.println(motorCalibrationRightHoldPwm_);
  out.print(F("  hold_ms="));
  out.println(kMotorCalibrationHoldMs);
  out.print(F("  brake_pwm="));
  out.println(kMotorCalibrationBrakePwm);
  out.print(F("  brake_ms="));
  out.println(kMotorCalibrationBrakeMs);
}

void HardwareSelfTestService::stopMotorPulse() {
  cancelAutoMotorCalibration();
  motors_.stop();
  Serial.println(F("motor gyro output stage=command_stop robot_left_pwm=0 robot_right_pwm=0 viewer_left_pwm=0 viewer_right_pwm=0 correction=0"));
  motorStopAtMs_ = 0;
  manualMotorTimeoutMs_ = 0;
  motorCalibrationNextMs_ = 0;
  motorCalibrationPhase_ = 0;
}

}  // namespace tongdou
