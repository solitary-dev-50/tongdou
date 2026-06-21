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
  return result;
}

void HardwareSelfTestService::printHelp(Print& out) const {
  out.println(F("self-test commands:"));
  out.println(F("  selftest"));
  out.println(F("  status"));
  out.println(F("  battery"));
  out.println(F("  radar"));
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

void HardwareSelfTestService::printMic(Print& out) {
  const AudioInputSnapshot snapshot = audioInput_.readLevel();
  out.println(F("mic:"));
  printBool(out, F("  ready="), snapshot.ready);
  printBool(out, F("  has_samples="), snapshot.hasSamples);
  out.print(F("  samples="));
  out.println(snapshot.samplesRead);
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
