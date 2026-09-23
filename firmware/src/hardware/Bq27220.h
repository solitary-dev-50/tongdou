#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace tongdou {

struct Bq27220Snapshot {
  bool valid = false;
  bool lowBattery = false;
  bool sleeping = false;
  uint16_t voltageMv = 0;
  int16_t currentMa = 0;
  int16_t averageCurrentMa = 0;
  uint16_t remainingCapacityMah = 0;
  uint16_t fullChargeCapacityMah = 0;
  uint16_t stateOfChargePercent = 0;
  uint16_t stateOfHealthPercent = 0;
  int32_t temperatureCentiC = 0;
  uint16_t batteryStatus = 0;
  uint32_t sampledAtMs = 0;
  uint32_t interruptCount = 0;
  uint32_t socChangeCount = 0;
  uint32_t lowBatteryEventCount = 0;
};

// 只负责 BQ27220 通信、可擦除运行参数和采样；不决定显示、提醒或电机动作。
class Bq27220 {
 public:
  explicit Bq27220(TwoWire& wire) : wire_(wire) {}
  void begin();
  void update();
  Bq27220Snapshot snapshot() const;
  const char* lastError() const;

 private:
  enum class State : uint8_t {
    Off, Probe, Identity, Security, UnsealFirst, UnsealSecond,
    FullFirst, FullSecond, CheckAccess, ConfigSelect, ConfigRead,
    EnterConfig, WaitConfig, ConfigWrite, ConfigCommit,
    VerifySelect, VerifyRead, ExitConfig, WaitExit, Seal, CheckSeal,
    BatteryPresence, InsertBattery, WaitReady, Idle, Sample,
    CleanupExit, CleanupSeal, Retry, WakeLow, WakeRelease
  };
  static void ARDUINO_ISR_ATTR onGpout(void* context);
  void advance(State state, uint32_t waitMs = 2);
  void fail(const char* reason);
  bool writeBytes(uint8_t reg, const uint8_t* bytes, uint8_t length);
  bool readBytes(uint8_t reg, uint8_t* bytes, uint8_t length);
  bool command(uint16_t value, uint8_t reg = 0x3E);
  bool readWord(uint8_t reg, uint16_t& value);
  bool readConfigValue(uint16_t& value);
  void nextConfigWrite();
  void startSample();
  void finishSample();

  TwoWire& wire_;
  State state_ = State::Off;
  Bq27220Snapshot data_;
  Bq27220Snapshot pending_;
  const char* error_ = "initializing";
  uint32_t nextAtMs_ = 0;
  uint32_t timeoutAtMs_ = 0;
  uint32_t sampleStartedAtMs_ = 0;
  uint32_t lastPollAtMs_ = 0;
  uint32_t lastLogAtMs_ = 0;
  volatile uint32_t interruptCount_ = 0;
  uint32_t handledInterruptCount_ = 0;
  uint16_t oldConfig_[8] = {};
  uint16_t newConfig_[8] = {};
  uint8_t configIndex_ = 0;
  uint8_t sampleIndex_ = 0;
  uint8_t lastRegister_ = 0;
  bool resetCapacity_ = false;
  bool inConfig_ = false;
  bool accessOpened_ = false;
};

}  // namespace tongdou
