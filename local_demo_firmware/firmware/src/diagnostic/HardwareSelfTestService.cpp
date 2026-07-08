#include "diagnostic/HardwareSelfTestService.h"

namespace tongdou {
namespace {

constexpr unsigned long kMotorPulseMs = 120;
constexpr uint16_t kSpeakerTestHz = 880;
constexpr uint16_t kSpeakerTestMs = 120;

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

}  // namespace

HardwareSelfTestService::HardwareSelfTestService(BatteryMonitor& battery,
                                                 AudioInput& audioInput,
                                                 AudioOutput& audioOutput,
                                                 LedPixel& led,
                                                 MotorDriver& motors,
                                                 RadarSensor& radar,
                                                 FaceDisplay& faceDisplay)
    : battery_(battery),
      audioInput_(audioInput),
      audioOutput_(audioOutput),
      led_(led),
      motors_(motors),
      radar_(radar),
      faceDisplay_(faceDisplay) {}

void HardwareSelfTestService::begin() {
  motorStopAtMs_ = 0;
}

void HardwareSelfTestService::update() {
  if (motorStopAtMs_ == 0 || millis() < motorStopAtMs_) {
    return;
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
  out.println(F("  led red|green|blue|off"));
  out.println(F("  motor forward|reverse|stop"));
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
  printBool(out, F("  reminders_available="), snapshot.remindersAvailable);
  printBool(out, F("  web_available="), snapshot.webAvailable);
  printBool(out, F("  visual_feedback_available="), snapshot.visualFeedbackAvailable);
  printBool(out, F("  sound_available="), snapshot.soundAvailable);
  printBool(out, F("  voice_input_available="), snapshot.voiceInputAvailable);
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

void HardwareSelfTestService::testLed(const RgbColor& color, Print& out) {
  led_.show(color);
  out.println(F("led test color set"));
}

void HardwareSelfTestService::testMotor(WheelDrive left, WheelDrive right, Print& out) {
  motors_.drive(left, right);
  motorStopAtMs_ = millis() + kMotorPulseMs;
  out.println(F("motor pulse started"));
}

void HardwareSelfTestService::stopMotorPulse() {
  motors_.stop();
  motorStopAtMs_ = 0;
}

}  // namespace tongdou
