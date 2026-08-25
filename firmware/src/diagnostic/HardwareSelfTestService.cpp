#include "diagnostic/HardwareSelfTestService.h"

#include <Wire.h>
#include <cstring>

namespace tongdou {
namespace {

constexpr unsigned long kMotorPulseMs = 120;
constexpr unsigned long kGyroMotorTestStopMs = 1200;
constexpr uint16_t kSpeakerTestHz = 880;
constexpr uint16_t kSpeakerTestMs = 120;
constexpr uint8_t kQmiStatus0 = 0x2E;
constexpr uint8_t kQmiTimestampL = 0x30;
constexpr uint8_t kQmiTempL = 0x33;

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

bool logoTouched(const LogoTouchInput& logoTouch) {
  const int32_t delta = logoTouch.delta();
  const uint32_t absDelta =
      delta >= 0 ? static_cast<uint32_t>(delta) : static_cast<uint32_t>(-delta);
  return absDelta >= logoTouch.threshold();
}

}  // namespace

HardwareSelfTestService::HardwareSelfTestService(BatteryMonitor& battery,
                                                 AudioInput& audioInput,
                                                 AudioOutput& audioOutput,
                                                 AudioCuePlayer& audioCuePlayer,
                                                 LedPixel& led,
                                                 MotorDriver& motors,
                                                 RadarSensor& radar,
                                                 FaceDisplay& faceDisplay,
                                                 ImuSensor& imu,
                                                 LogoTouchInput& logoTouch)
    : battery_(battery),
      audioInput_(audioInput),
      audioOutput_(audioOutput),
      audioCuePlayer_(audioCuePlayer),
      led_(led),
      motors_(motors),
      radar_(radar),
      faceDisplay_(faceDisplay),
      imu_(imu),
      logoTouch_(logoTouch),
      gyroMotion_(motors_, imu_) {}

void HardwareSelfTestService::begin() {
  motorStopAtMs_ = 0;
  gyroMotion_.begin();
}

void HardwareSelfTestService::update() {
  gyroMotion_.update();
  if (motorStopAtMs_ == 0 || millis() < motorStopAtMs_) {
    return;
  }

  stopMotorPulse();
}

HardwareDiagnosticStatus HardwareSelfTestService::status() const {
  HardwareDiagnosticStatus result;
  result.faceReady = faceDisplay_.ready();
  result.imuReady = imu_.ready();
  result.logoTouchReady = logoTouch_.ready();
  result.ledReady = led_.ready();
  result.audioInputReady = audioInput_.ready();
  result.audioOutputReady = audioOutput_.ready();
  result.motorFaultActive = motors_.faultActive();
  result.motorAvailable = motors_.available();
  result.motorFaultSimulated = motors_.faultSimulated();
  result.degraded = !result.faceReady || !result.ledReady || !result.audioInputReady ||
                    !result.audioOutputReady || result.motorFaultActive;
  result.visualFeedbackAvailable = result.faceReady || result.ledReady;
  result.soundAvailable = result.audioOutputReady;
  result.voiceInputAvailable = result.audioInputReady;
  return result;
}

BatterySnapshot HardwareSelfTestService::batterySnapshot() const {
  return battery_.read();
}

void HardwareSelfTestService::printHelp(Print& out) const {
  out.println(F("self-test commands:"));
  out.println(F("  selftest"));
  out.println(F("  status"));
  out.println(F("  battery"));
  out.println(F("  i2c scan"));
  out.println(F("  imu"));
  out.println(F("  logo touch"));
  out.println(F("  radar"));
  out.println(F("  radar target"));
  out.println(F("  radar sample"));
  out.println(F("  radar config"));
  out.println(F("  radar near"));
  out.println(F("  radar resolution"));
  out.println(F("  radar res20"));
  out.println(F("  radar calibrate"));
  out.println(F("  mic"));
  out.println(F("  speaker"));
  out.println(F("  audio cue wake|success|error|low_battery|wifi|welcome|offline [locale]"));
  out.println(F("  audio code <digits> [locale]"));
  out.println(F("  led red|green|blue|off"));
  out.println(F("  motor fault"));
  out.println(F("  motor fault simulate on|off"));
  out.println(F("  motor forward|reverse|turn left|turn right|stop"));
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
  if (cmd == F("i2c scan")) {
    printI2cScan(out);
    return true;
  }
  if (cmd == F("imu")) {
    printImu(out);
    return true;
  }
  if (cmd == F("logo touch")) {
    printLogoTouch(out);
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
  if (cmd == F("radar config")) {
    printRadarConfig(out);
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
  if (cmd == F("mic")) {
    printMic(out);
    return true;
  }
  if (cmd == F("speaker")) {
    testSpeaker(out);
    return true;
  }
  if (cmd.startsWith(F("audio cue ")) || cmd.startsWith(F("audio code "))) {
    testAudioCue(cmd, out);
    return true;
  }
  if (cmd == F("motor fault")) {
    printMotorFault(out);
    return true;
  }
  if (cmd == F("motor fault simulate on")) {
    setMotorFaultSimulation(true, out);
    return true;
  }
  if (cmd == F("motor fault simulate off")) {
    setMotorFaultSimulation(false, out);
    return true;
  }
  if (cmd == F("led red")) {
    testLed({64, 0, 0}, out);
    return true;
  }
  if (cmd == F("led green")) {
    testLed({0, 64, 0}, out);
    return true;
  }
  if (cmd == F("led blue")) {
    testLed({0, 0, 64}, out);
    return true;
  }
  if (cmd == F("led off")) {
    led_.off();
    out.println(F("led off"));
    return true;
  }
  if (cmd == F("motor forward")) {
    testMotor(WheelDrive::Forward, WheelDrive::Forward, out);
    return true;
  }
  if (cmd == F("motor reverse")) {
    testMotor(WheelDrive::Reverse, WheelDrive::Reverse, out);
    return true;
  }
  if (cmd == F("motor turn left")) {
    testMotor(WheelDrive::Reverse, WheelDrive::Forward, out);
    return true;
  }
  if (cmd == F("motor turn right")) {
    testMotor(WheelDrive::Forward, WheelDrive::Reverse, out);
    return true;
  }
  if (cmd == F("motor stop")) {
    stopMotorPulse();
    out.println(F("motor stop"));
    return true;
  }

  return false;
}

void HardwareSelfTestService::printReport(Print& out) {
  out.println(F("self-test report begin"));
  printStatus(out);
  printBattery(out);
  printI2cScan(out);
  printImu(out);
  printLogoTouch(out);
  printRadar(out);
  printMic(out);
  out.println(F("self-test report end"));
}

void HardwareSelfTestService::printStatus(Print& out) const {
  const HardwareDiagnosticStatus snapshot = status();
  out.println(F("hardware:"));
  printBool(out, F("  face="), snapshot.faceReady);
  printBool(out, F("  imu="), snapshot.imuReady);
  printBool(out, F("  logo_touch="), snapshot.logoTouchReady);
  printBool(out, F("  led="), snapshot.ledReady);
  printBool(out, F("  audio_input="), snapshot.audioInputReady);
  printBool(out, F("  audio_output="), snapshot.audioOutputReady);
  printBool(out, F("  motor_fault="), snapshot.motorFaultActive);
  printBool(out, F("  motor_fault_simulated="), snapshot.motorFaultSimulated);
  printBool(out, F("  motor_available="), snapshot.motorAvailable);
  printBool(out, F("  degraded="), snapshot.degraded);
  printBool(out, F("  reminders_available="), snapshot.remindersAvailable);
  printBool(out, F("  web_available="), snapshot.webAvailable);
  printBool(out, F("  visual_feedback_available="), snapshot.visualFeedbackAvailable);
  printBool(out, F("  sound_available="), snapshot.soundAvailable);
  printBool(out, F("  voice_input_available="), snapshot.voiceInputAvailable);
}

void HardwareSelfTestService::printI2cScan(Print& out) const {
  out.println(F("i2c scan:"));
  uint8_t count = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) {
      continue;
    }

    out.print(F("  found=0x"));
    printHexByte(out, address);
    if (address == 0x3C) {
      out.print(F(" oled_candidate"));
    } else if (address == 0x6A || address == 0x6B) {
      out.print(F(" qmi8658a_candidate"));
    }
    out.println();
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
  if (!gyro.ready) {
    return;
  }

  out.print(F("  gyro_raw_x="));
  out.println(gyro.xRaw);
  out.print(F("  gyro_raw_y="));
  out.println(gyro.yRaw);
  out.print(F("  gyro_raw_z="));
  out.println(gyro.zRaw);
  out.print(F("  gyro_mdps_x="));
  out.println(gyro.xMdegPerSec);
  out.print(F("  gyro_mdps_y="));
  out.println(gyro.yMdegPerSec);
  out.print(F("  gyro_mdps_z="));
  out.println(gyro.zMdegPerSec);

  const uint8_t registers[] = {kQmiStatus0, kQmiTimestampL, kQmiTempL};
  out.print(F("  debug="));
  for (uint8_t i = 0; i < sizeof(registers); ++i) {
    uint8_t value = 0;
    if (i > 0) {
      out.print(' ');
    }
    out.print(F("0x"));
    printHexByte(out, registers[i]);
    out.print('=');
    if (imu_.readDebugRegister(registers[i], value)) {
      out.print(F("0x"));
      printHexByte(out, value);
    } else {
      out.print(F("read_failed"));
    }
  }
  out.println();
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

void HardwareSelfTestService::printBattery(Print& out) const {
  const BatterySnapshot snapshot = battery_.read();
  out.println(F("battery:"));
  printBool(out, F("  usb="), snapshot.usbPresent);
  printBool(out, F("  charging="), snapshot.charging);
  printBool(out, F("  standby="), snapshot.standby);
  out.print(F("  raw_adc="));
  out.println(snapshot.rawAdc);
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
  if (!snapshot.received) {
    return;
  }

  out.print(F("  state="));
  out.println(snapshot.targetState);
  out.print(F("  raw_payload="));
  for (uint8_t i = 0; i < snapshot.rawPayloadLength; ++i) {
    if (i > 0) {
      out.print(' ');
    }
    printHexByte(out, snapshot.rawPayload[i]);
  }
  out.println();
  printBool(out, F("  has_target="), snapshot.hasTarget);
  out.println(F("  distance_note=raw_auxiliary_not_real_distance"));
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
}

void HardwareSelfTestService::printRadarSamples(Print& out) {
  out.println(F("radar sample:"));
  out.println(F("  distance_note=raw_auxiliary_not_real_distance"));
  for (uint8_t sample = 1; sample <= 8; ++sample) {
    const RadarTargetSnapshot snapshot = radar_.readTarget();
    out.print(F("  sample="));
    out.print(sample);
    printBool(out, F(" received="), snapshot.received);
    if (snapshot.received) {
      out.print(F("    state="));
      out.print(snapshot.targetState);
      out.print(F(" has_target="));
      out.print(snapshot.hasTarget ? F("1") : F("0"));
      out.print(F(" moving_cm="));
      out.print(snapshot.movingDistanceCm);
      out.print(F(" moving_energy="));
      out.print(snapshot.movingEnergy);
      out.print(F(" static_cm="));
      out.print(snapshot.staticDistanceCm);
      out.print(F(" static_energy="));
      out.println(snapshot.staticEnergy);
    }
    delay(120);
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

void HardwareSelfTestService::applyRadarNearConfig(Print& out) {
  const bool ok = radar_.applyNearDeskConfig();
  out.println(ok ? F("radar near config applied") : F("radar near config failed"));
  if (ok) {
    printRadarConfig(out);
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
    out.println(F("  note=keep the front empty for at least 10 seconds"));
  }
}

void HardwareSelfTestService::printMic(Print& out) {
  const AudioInputSnapshot snapshot = audioInput_.readLevel();
  out.println(F("mic:"));
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

void HardwareSelfTestService::testSpeaker(Print& out) {
  audioOutput_.playTestTone(kSpeakerTestHz, kSpeakerTestMs);
  out.println(F("speaker test tone done"));
}

void HardwareSelfTestService::testAudioCue(const String& command, Print& out) {
  String rest = command;
  rest.trim();

  if (rest.startsWith(F("audio code "))) {
    rest.remove(0, strlen("audio code "));
    rest.trim();
    const int split = rest.indexOf(' ');
    const String code = split < 0 ? rest : rest.substring(0, split);
    const String locale = split < 0 ? "en-GB" : rest.substring(split + 1);
    printAudioCueResult(audioCuePlayer_.playActivationCode(code, locale), out);
    return;
  }

  rest.remove(0, strlen("audio cue "));
  rest.trim();
  const int split = rest.indexOf(' ');
  const String cueName = split < 0 ? rest : rest.substring(0, split);
  const String locale = split < 0 ? "en-GB" : rest.substring(split + 1);

  AudioCuePlaybackResult result;
  if (cueName == F("wake")) {
    result = audioCuePlayer_.playSystem(SystemAudioCue::Wake);
  } else if (cueName == F("success")) {
    result = audioCuePlayer_.playSystem(SystemAudioCue::Success);
  } else if (cueName == F("error")) {
    result = audioCuePlayer_.playSystem(SystemAudioCue::Error);
  } else if (cueName == F("low_battery")) {
    result = audioCuePlayer_.playSystem(SystemAudioCue::LowBattery);
  } else if (cueName == F("vibration")) {
    result = audioCuePlayer_.playSystem(SystemAudioCue::Vibration);
  } else if (cueName == F("wifi")) {
    result = audioCuePlayer_.playLocalized(LocalizedAudioCue::WifiConfig, locale);
  } else if (cueName == F("welcome")) {
    result = audioCuePlayer_.playLocalized(LocalizedAudioCue::Welcome, locale);
  } else if (cueName == F("activation")) {
    result = audioCuePlayer_.playLocalized(LocalizedAudioCue::ActivationCode, locale);
  } else if (cueName == F("offline")) {
    const OfflineNoticePlaybackResult offline =
        audioCuePlayer_.playOfflineNotice(locale);
    printAudioCueResult(offline.speech, out);
    printBool(out, F("  speech_drained="), offline.speechDrained);
    out.print(F("  tones_played="));
    out.println(offline.tonesPlayed);
    return;
  } else {
    out.println(F("audio cue unknown"));
    return;
  }

  printAudioCueResult(result, out);
}

void HardwareSelfTestService::printAudioCueResult(const AudioCuePlaybackResult& result,
                                                  Print& out) const {
  out.println(F("audio cue:"));
  printBool(out, F("  ok="), result.ok);
  out.print(F("  packets="));
  out.println(result.packets);
  out.print(F("  samples="));
  out.println(result.samples);
  out.print(F("  sample_rate="));
  out.println(result.sampleRate);
  if (!result.ok) {
    out.print(F("  error="));
    out.println(result.error);
  }
}

void HardwareSelfTestService::printMotorFault(Print& out) const {
  out.println(F("motor fault:"));
  printBool(out, F("  fault_active="), motors_.faultActive());
  printBool(out, F("  fault_pin_low="), motors_.faultPinLow());
  printBool(out, F("  simulated="), motors_.faultSimulated());
  printBool(out, F("  available="), motors_.available());
}

void HardwareSelfTestService::setMotorFaultSimulation(bool active, Print& out) {
  motors_.setFaultSimulation(active);
  out.println(active ? F("motor fault simulation on") : F("motor fault simulation off"));
  printMotorFault(out);
}

void HardwareSelfTestService::testLed(const RgbColor& color, Print& out) {
  led_.show(color);
  out.println(F("led test color set"));
}

void HardwareSelfTestService::testMotor(WheelDrive left, WheelDrive right, Print& out) {
  if (motors_.faultActive()) {
    stopMotorPulse();
    out.println(F("motor pulse blocked reason=fault"));
    printMotorFault(out);
    return;
  }

  diagnosticMotionFrame_ = {left, right, kMotorPulseMs, 190};
  gyroMotion_.start({&diagnosticMotionFrame_, 1});
  motorStopAtMs_ = millis() + kGyroMotorTestStopMs;
  out.println(F("gyro motor pulse started"));
}

void HardwareSelfTestService::stopMotorPulse() {
  gyroMotion_.stop();
  motorStopAtMs_ = 0;
}

}  // namespace tongdou
