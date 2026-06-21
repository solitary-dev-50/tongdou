#include "hardware/HardwareManager.h"

namespace tongdou {

void HardwareManager::begin() {
  battery_.begin();
  displayBus_.begin();
  faceDisplay_.begin();
  led_.begin();
  motors_.begin();
  radar_.begin();
  audioInput_.begin();
  audioOutput_.begin();
  selfTest_.begin();
}

void HardwareManager::update() {
  audioInput_.update();
  selfTest_.update();
}

BatteryMonitor& HardwareManager::battery() {
  return battery_;
}

FaceDisplay& HardwareManager::faceDisplay() {
  return faceDisplay_;
}

LedPixel& HardwareManager::led() {
  return led_;
}

MotorDriver& HardwareManager::motors() {
  return motors_;
}

RadarSensor& HardwareManager::radar() {
  return radar_;
}

AudioInput& HardwareManager::audioInput() {
  return audioInput_;
}

AudioOutput& HardwareManager::audioOutput() {
  return audioOutput_;
}

HardwareSelfTestService& HardwareManager::selfTest() {
  return selfTest_;
}

}  // namespace tongdou
