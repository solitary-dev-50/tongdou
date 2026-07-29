#include "hardware/ImuSensor.h"

#include <Wire.h>

namespace tongdou {
namespace {

constexpr uint8_t kAddressLow = 0x6A;
constexpr uint8_t kAddressHigh = 0x6B;
constexpr uint8_t kWhoAmI = 0x00;
constexpr uint8_t kExpectedWhoAmI = 0x05;
constexpr uint8_t kCtrl1 = 0x02;
constexpr uint8_t kCtrl2 = 0x03;
constexpr uint8_t kCtrl3 = 0x04;
constexpr uint8_t kCtrl5 = 0x06;
constexpr uint8_t kCtrl7 = 0x08;
constexpr uint8_t kReset = 0x60;
constexpr uint8_t kResetCommand = 0xB0;
constexpr uint8_t kTempOutL = 0x33;
constexpr uint8_t kGyroXoutL = 0x3B;
constexpr uint8_t kTempAccelGyroReadLength = 14;
constexpr uint8_t kGyroOffset = kGyroXoutL - kTempOutL;
constexpr int32_t kGyroLsbPerDps = 64;  // 512 dps range.

int16_t readInt16Le(const uint8_t* buffer) {
  return static_cast<int16_t>((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]);
}

}  // namespace

void ImuSensor::begin() {
  ready_ = false;
  address_ = 0;
  whoAmI_ = 0;

  if (!probeAddress(kAddressHigh) && !probeAddress(kAddressLow)) {
    Serial.println(F("imu qmi8658a not found"));
    return;
  }

  if (!writeRegister(kReset, kResetCommand)) {
    Serial.println(F("imu qmi8658a reset failed"));
    ready_ = false;
    return;
  }
  delay(20);

  // QMI8658A basic config: I2C auto-increment, accel 8g/125Hz, gyro 512dps/125Hz.
  if (!writeRegister(kCtrl1, 0x40) || !writeRegister(kCtrl2, 0x26) ||
      !writeRegister(kCtrl3, 0x56) || !writeRegister(kCtrl5, 0x00) ||
      !writeRegister(kCtrl7, 0x03)) {
    Serial.println(F("imu qmi8658a config failed"));
    ready_ = false;
    return;
  }

  delay(20);
  ready_ = true;
  Serial.print(F("imu qmi8658a ready addr=0x"));
  Serial.print(address_, HEX);
  Serial.print(F(" who=0x"));
  Serial.println(whoAmI_, HEX);
}

bool ImuSensor::ready() const {
  return ready_;
}

uint8_t ImuSensor::address() const {
  return address_;
}

uint8_t ImuSensor::whoAmI() const {
  return whoAmI_;
}

ImuGyroSnapshot ImuSensor::readGyro() {
  ImuGyroSnapshot snapshot;
  snapshot.ready = ready_;
  if (!ready_) {
    return snapshot;
  }

  uint8_t buffer[kTempAccelGyroReadLength] = {};
  if (!readRegisters(kTempOutL, buffer, sizeof(buffer))) {
    snapshot.ready = false;
    return snapshot;
  }

  snapshot.xRaw = readInt16Le(&buffer[kGyroOffset + 0]);
  snapshot.yRaw = readInt16Le(&buffer[kGyroOffset + 2]);
  snapshot.zRaw = readInt16Le(&buffer[kGyroOffset + 4]);
  snapshot.xMdegPerSec =
      static_cast<int32_t>(snapshot.xRaw) * 1000L / kGyroLsbPerDps;
  snapshot.yMdegPerSec =
      static_cast<int32_t>(snapshot.yRaw) * 1000L / kGyroLsbPerDps;
  snapshot.zMdegPerSec =
      static_cast<int32_t>(snapshot.zRaw) * 1000L / kGyroLsbPerDps;
  return snapshot;
}

bool ImuSensor::readDebugRegister(uint8_t reg, uint8_t& value) const {
  return readRegister(reg, value);
}

bool ImuSensor::readDebugRegisters(uint8_t startReg, uint8_t* buffer,
                                   uint8_t length) const {
  return readRegisters(startReg, buffer, length);
}

bool ImuSensor::probeAddress(uint8_t address) {
  address_ = address;
  uint8_t who = 0;
  if (!readRegister(kWhoAmI, who)) {
    return false;
  }
  if (who != kExpectedWhoAmI) {
    Serial.print(F("imu unexpected who addr=0x"));
    Serial.print(address, HEX);
    Serial.print(F(" who=0x"));
    Serial.println(who, HEX);
    return false;
  }

  whoAmI_ = who;
  return true;
}

bool ImuSensor::readRegister(uint8_t reg, uint8_t& value) const {
  return readRegisters(reg, &value, 1);
}

bool ImuSensor::readRegisters(uint8_t startReg, uint8_t* buffer, uint8_t length) const {
  if (address_ == 0 || buffer == nullptr || length == 0) {
    return false;
  }

  Wire.beginTransmission(address_);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t received = Wire.requestFrom(static_cast<int>(address_),
                                            static_cast<int>(length));
  if (received != length) {
    return false;
  }

  for (uint8_t i = 0; i < length; ++i) {
    buffer[i] = Wire.read();
  }
  return true;
}

bool ImuSensor::writeRegister(uint8_t reg, uint8_t value) const {
  if (address_ == 0) {
    return false;
  }

  Wire.beginTransmission(address_);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

}  // namespace tongdou
