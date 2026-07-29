#pragma once

#include <Arduino.h>

namespace tongdou {

struct ImuGyroSnapshot {
  bool ready = false;
  int16_t xRaw = 0;
  int16_t yRaw = 0;
  int16_t zRaw = 0;
  int32_t xMdegPerSec = 0;
  int32_t yMdegPerSec = 0;
  int32_t zMdegPerSec = 0;
};

class ImuSensor {
 public:
  void begin();
  bool ready() const;
  uint8_t address() const;
  uint8_t whoAmI() const;
  ImuGyroSnapshot readGyro();
  bool readDebugRegister(uint8_t reg, uint8_t& value) const;
  bool readDebugRegisters(uint8_t startReg, uint8_t* buffer, uint8_t length) const;

 private:
  bool probeAddress(uint8_t address);
  bool readRegister(uint8_t reg, uint8_t& value) const;
  bool readRegisters(uint8_t startReg, uint8_t* buffer, uint8_t length) const;
  bool writeRegister(uint8_t reg, uint8_t value) const;

  bool ready_ = false;
  uint8_t address_ = 0;
  uint8_t whoAmI_ = 0;
};

}  // namespace tongdou
