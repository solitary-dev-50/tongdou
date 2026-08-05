#include "hardware/HardwareManager.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {

void HardwareManager::begin() {
  motors_.begin();

  battery_.begin();
  startupBattery_ = battery_.read();

  displayBus_.begin();

  faceDisplay_.begin();
  imu_.begin();

  led_.begin();
  logoTouch_.begin();

  radar_.begin();

  audioInput_.begin();
  audioOutput_.begin();
  selfTest_.begin();
}

void HardwareManager::update() {
  logoTouch_.update();
  audioInput_.update();
  selfTest_.update();
}

void HardwareManager::printStartupReport(Print& out) const {
  out.println("hardware begin");
  out.println("  motor stop");

  out.print("  battery ready usb=");
  out.print(startupBattery_.usbPresent ? "1" : "0");
  out.print(" chrg=");
  out.print(startupBattery_.charging ? "1" : "0");
  out.print(" stby=");
  out.print(startupBattery_.standby ? "1" : "0");
  out.print(" vbat_adc=");
  out.println(startupBattery_.rawAdc);

  out.println("  i2c ready");
  out.println(faceDisplay_.ready() ? "  display ready" : "  display failed");
  if (imu_.ready()) {
    out.print("  imu ready addr=0x");
    out.print(imu_.address(), HEX);
    out.print(" who=0x");
    out.println(imu_.whoAmI(), HEX);
  } else {
    out.println("  imu failed");
  }
  out.println(led_.ready() ? "  led off" : "  led failed");
  out.print("  logo touch ");
  out.print(logoTouch_.ready() ? "ready" : "failed");
  out.print(" pin=");
  out.print(static_cast<int>(pins::LOGO_TOUCH));
  out.print(" raw=");
  out.print(logoTouch_.raw());
  out.print(" baseline=");
  out.print(logoTouch_.baseline());
  out.print(" threshold=");
  out.println(logoTouch_.threshold());
  out.println("  radar ready");

  out.print("  mic ");
  out.print(audioInput_.ready() ? "ready" : "failed");
  out.print(" pdm_clk=");
  out.print(static_cast<int>(pins::PDM_MIC_CLK));
  out.print(" data=");
  out.println(static_cast<int>(pins::PDM_MIC_DATA));

  out.print("  speaker ");
  out.print(audioOutput_.ready() ? "ready" : "failed");
  out.print(" bclk=");
  out.print(static_cast<int>(pins::I2S_SPK_BCLK));
  out.print(" lrclk=");
  out.print(static_cast<int>(pins::I2S_SPK_LRCLK));
  out.print(" data=");
  out.print(static_cast<int>(pins::I2S_SPK_DATA));
  out.print(" ctrl=");
  out.println(static_cast<int>(pins::SPK_CTRL));

  out.println("hardware ready");
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

ImuSensor& HardwareManager::imu() {
  return imu_;
}

LogoTouchInput& HardwareManager::logoTouch() {
  return logoTouch_;
}

HardwareSelfTestService& HardwareManager::selfTest() {
  return selfTest_;
}

}  // namespace tongdou
