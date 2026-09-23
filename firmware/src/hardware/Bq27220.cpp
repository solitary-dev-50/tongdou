#include "hardware/Bq27220.h"

#include "tongdou/BatteryConfig.h"
#include "tongdou/BoardProfile.h"
#include "tongdou/Pins.h"

#if TONGDOU_BOARD_REVISION == 94
namespace tongdou {
namespace {
using namespace battery_config;
constexpr uint8_t kAddress = 0x55;
constexpr uint16_t kDeviceNumber = 0x0220;
constexpr uint16_t kConfigUpdate = 0x0400;
constexpr uint16_t kInitComplete = 0x0020;
constexpr uint16_t kSecurityMask = 0x0006;
constexpr uint16_t kSealed = 0x0006;
constexpr uint16_t kFullAccess = 0x0002;
constexpr uint16_t kBatteryPresent = 0x0008;
constexpr uint16_t kLowFlags = 0x8006;  // FD / TDA / SYSDWN。
constexpr uint16_t kI2cTimeoutMs = 10;  // 官方允许最多约 4ms 的时钟延展。
constexpr uint32_t kConfigWaitMs = 2000;
constexpr uint32_t kStateTimeoutMs = 10000;

struct RamParameter {
  uint16_t address;
  uint16_t value;
  uint16_t preserveMask;
  uint8_t size;
};

// 仅使用 RAM 地址。地址小端、参数大端，见 TI SLUUBD4A 第 6 章及官方勘误。
// FCC 必须先写、Design Capacity 最后写：失败重试时仍能识别未完成的容量迁移。
constexpr RamParameter kParameters[] = {
    {0x929D, kDesignCapacityMah, 0, 2},  // 仅初次容量迁移设置，之后保留学习结果。
    {0x9206, 0, 0x6E7F, 2},           // 内温、主机电池在位；禁用 BAT_GD。
    {0x9208, 0x0060, 0xFF9D, 2},      // 低电量/充放电变化通知；INT_POL=0。
    {0x920D, 0, 0x00FC, 1},           // 禁用独占 GPOUT 的 BTP 模式。
    {0x920B, kSocDeltaPercent, 0, 1},  // 电量变化 1% 通知。
    {0x91FD, kChargingVoltageMv, 0, 2},
    {0x9221, 0, 0, 1},               // TI 要求关闭此芯片不支持的 Hibernate I。
    {0x929F, kDesignCapacityMah, 0, 2},
};
constexpr uint8_t kParameterCount = sizeof(kParameters) / sizeof(kParameters[0]);
constexpr uint8_t kSampleRegisters[] = {
    0x3A, 0x3C, 0x06, 0x08, 0x0A, 0x0C, 0x10, 0x12, 0x14, 0x2C, 0x2E};

uint16_t littleEndian(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
}
}  // namespace

void ARDUINO_ISR_ATTR Bq27220::onGpout(void* context) {
  // ISR 只递增计数。主循环不清零，32 位对齐读取，避免清标志时丢掉新脉冲。
  ++static_cast<Bq27220*>(context)->interruptCount_;
}

void Bq27220::begin() {
  pinMode(pins::BQ27220_GPOUT, INPUT);  // 板上已有 10kΩ 上拉。
  Serial.printf("bq27220: init addr=0x55 gpout=%d active=low capacity=%umAh charge=%umV rsense=10mOhm temp=internal ram_only\n",
                static_cast<int>(pins::BQ27220_GPOUT), kDesignCapacityMah, kChargingVoltageMv);
  advance(State::WakeLow, 0);
}

void Bq27220::advance(State state, uint32_t waitMs) {
  state_ = state;
  nextAtMs_ = static_cast<uint32_t>(millis()) + waitMs;
}

void Bq27220::fail(const char* reason) {
  if (state_ == State::CleanupExit || state_ == State::CleanupSeal) return;
  error_ = reason;
  data_.valid = false;
  Serial.printf("bq27220: fault=%s state=%u reg=0x%02X; battery source=adc retry_ms=%lu\n",
                reason, static_cast<unsigned>(state_), lastRegister_,
                static_cast<unsigned long>(kRetryIntervalMs));
  advance(inConfig_ ? State::CleanupExit : State::CleanupSeal);
}

bool Bq27220::writeBytes(uint8_t reg, const uint8_t* bytes, uint8_t length) {
  lastRegister_ = reg;
  const uint16_t previousTimeout = wire_.getTimeOut();
  wire_.setTimeOut(kI2cTimeoutMs);
  wire_.beginTransmission(kAddress);
  wire_.write(reg);
  wire_.write(bytes, length);
  const uint8_t result = wire_.endTransmission();
  wire_.setTimeOut(previousTimeout);
  if (result != 0) {
    Serial.printf("bq27220: write failed addr=0x55 reg=0x%02X wire_code=%u (2=nack,4=other,5=timeout) gpout=%d sda=%d scl=%d\n",
                  reg, result, digitalRead(pins::BQ27220_GPOUT),
                  digitalRead(pins::I2C_SDA), digitalRead(pins::I2C_SCL));
    fail("i2c_write_failed");
    return false;
  }
  return true;
}

bool Bq27220::readBytes(uint8_t reg, uint8_t* bytes, uint8_t length) {
  lastRegister_ = reg;
  const uint16_t previousTimeout = wire_.getTimeOut();
  wire_.setTimeOut(kI2cTimeoutMs);
  wire_.beginTransmission(kAddress);
  wire_.write(reg);
  const uint8_t result = wire_.endTransmission(false);
  size_t received = 0;
  if (result == 0) received = wire_.requestFrom(kAddress, length);
  for (size_t i = 0; i < received; ++i) bytes[i] = static_cast<uint8_t>(wire_.read());
  wire_.setTimeOut(previousTimeout);
  if (result != 0 || received != length) {
    fail(result != 0 ? "i2c_read_failed" : "i2c_short_read");
    return false;
  }
  return true;
}

bool Bq27220::command(uint16_t value, uint8_t reg) {
  const uint8_t bytes[] = {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8)};
  return writeBytes(reg, bytes, sizeof(bytes));
}

bool Bq27220::readWord(uint8_t reg, uint16_t& value) {
  uint8_t bytes[2];
  if (!readBytes(reg, bytes, sizeof(bytes))) return false;
  value = littleEndian(bytes);
  return true;
}

bool Bq27220::readConfigValue(uint16_t& value) {
  const RamParameter& parameter = kParameters[configIndex_];
  uint8_t bytes[4];
  if (!readBytes(0x3E, bytes, parameter.size + 2)) return false;
  if (littleEndian(bytes) != parameter.address) {
    fail("ram_address_echo_mismatch");
    return false;
  }
  value = parameter.size == 1 ? bytes[2] :
      (static_cast<uint16_t>(bytes[2]) << 8) | bytes[3];
  return true;
}

void Bq27220::nextConfigWrite() {
  while (configIndex_ < kParameterCount &&
         (oldConfig_[configIndex_] == newConfig_[configIndex_] ||
          (configIndex_ == 0 && !resetCapacity_))) ++configIndex_;
  advance(configIndex_ == kParameterCount ? State::ExitConfig : State::ConfigWrite);
}

void Bq27220::startSample() {
  pending_ = {};
  sampleIndex_ = 0;
  sampleStartedAtMs_ = static_cast<uint32_t>(millis());
  lastPollAtMs_ = sampleStartedAtMs_;
  handledInterruptCount_ = interruptCount_;
  advance(State::Sample, 0);
}

void Bq27220::finishSample() {
  if (!(pending_.batteryStatus & kBatteryPresent) || pending_.voltageMv == 0 ||
      pending_.voltageMv > 6000 || pending_.stateOfChargePercent > 100 ||
      pending_.stateOfHealthPercent > 100 || pending_.fullChargeCapacityMah == 0 ||
      pending_.temperatureCentiC < -4000 || pending_.temperatureCentiC > 8500) {
    fail("invalid_measurement_or_battery_absent");
    return;
  }
  pending_.lowBattery = (pending_.batteryStatus & kLowFlags) != 0 ||
                        pending_.stateOfChargePercent <= kLowPercent;
  pending_.sleeping = (pending_.batteryStatus & 0x1000) != 0;
  pending_.socChangeCount = data_.socChangeCount +
      (data_.valid && pending_.stateOfChargePercent != data_.stateOfChargePercent ? 1 : 0);
  pending_.lowBatteryEventCount = data_.lowBatteryEventCount +
      (pending_.lowBattery && (!data_.valid || !data_.lowBattery) ? 1 : 0);
  pending_.interruptCount = interruptCount_;
  pending_.sampledAtMs = static_cast<uint32_t>(millis());
  pending_.valid = true;
  const bool recovered = !data_.valid;
  const bool event = pending_.socChangeCount != data_.socChangeCount ||
                     pending_.lowBatteryEventCount != data_.lowBatteryEventCount ||
                     pending_.interruptCount != data_.interruptCount;
  data_ = pending_;
  error_ = "";
  if (recovered) Serial.println(F("bq27220: init/readback ok; battery source=bq27220"));
  if (recovered || event || data_.sampledAtMs - lastLogAtMs_ >= 30000) {
    lastLogAtMs_ = data_.sampledAtMs;
    Serial.printf("bq27220: voltage=%umV current=%dmA average=%dmA soc=%u%% remaining=%umAh full=%umAh temp=%ldcentiC health=%u%% low=%u irq=%lu\n",
                  data_.voltageMv, data_.currentMa, data_.averageCurrentMa,
                  data_.stateOfChargePercent, data_.remainingCapacityMah,
                  data_.fullChargeCapacityMah, static_cast<long>(data_.temperatureCentiC),
                  data_.stateOfHealthPercent, data_.lowBattery,
                  static_cast<unsigned long>(data_.interruptCount));
  }
  advance(State::Idle);
}

void Bq27220::update() {
  const uint32_t now = static_cast<uint32_t>(millis());
  if (static_cast<int32_t>(now - nextAtMs_) < 0) return;
  uint16_t value = 0;
  switch (state_) {
    case State::Off: return;
    case State::WakeLow:
      // GPOUT 上升沿可退出 SHUTDOWN。只开漏拉低，随后由板上电阻拉高。
      // 暂停中断，避免把主机自己的唤醒脉冲当作电量通知。
      detachInterrupt(digitalPinToInterrupt(pins::BQ27220_GPOUT));
      digitalWrite(pins::BQ27220_GPOUT, LOW);
      pinMode(pins::BQ27220_GPOUT, OUTPUT_OPEN_DRAIN);
      advance(State::WakeRelease, 2);  // TI 要求至少 10us；分步等待 2ms。
      return;
    case State::WakeRelease:
      pinMode(pins::BQ27220_GPOUT, INPUT);
      attachInterruptArg(digitalPinToInterrupt(pins::BQ27220_GPOUT), onGpout, this, FALLING);
      Serial.println(F("bq27220: wake gpout low pulse sent; released to external pullup; wait_ms=300"));
      advance(State::Probe, 300);  // 覆盖内部电源建立及 250ms 通信就绪时间。
      return;
    case State::Probe:
      Serial.printf("bq27220: after wake gpout=%d sda=%d scl=%d (1=high,0=low)\n",
                    digitalRead(pins::BQ27220_GPOUT), digitalRead(pins::I2C_SDA),
                    digitalRead(pins::I2C_SCL));
      if (!command(0x0001)) return;
      advance(State::Identity, 5);
      return;
    case State::Identity: {
      uint8_t bytes[4];
      if (!readBytes(0x3E, bytes, sizeof(bytes))) return;
      if (littleEndian(bytes) != 0x0001 || littleEndian(bytes + 2) != kDeviceNumber) {
        fail("wrong_device_number");
        return;
      }
      advance(State::Security);
      return;
    }
    case State::Security:
      if (!readWord(0x3A, value)) return;
      inConfig_ = (value & kConfigUpdate) != 0;
      advance((value & kSecurityMask) == kSealed ? State::UnsealFirst : State::FullFirst);
      return;
    case State::UnsealFirst:
      // SLUUBD4A 第 6 章的增量写入顺序，非其他型号的解封命令。
      if (command(0x0414, 0x00)) advance(State::UnsealSecond);
      return;
    case State::UnsealSecond:
      if (command(0x3672, 0x00)) advance(State::FullFirst, 10);
      return;
    case State::FullFirst:
      accessOpened_ = true;
      if (command(0xFFFF, 0x00)) advance(State::FullSecond);
      return;
    case State::FullSecond:
      if (command(0xFFFF, 0x00)) advance(State::CheckAccess, 10);
      return;
    case State::CheckAccess:
      if (!readWord(0x3A, value)) return;
      if ((value & kSecurityMask) != kFullAccess) {
        fail("full_access_denied");
        return;
      }
      configIndex_ = 0;
      advance(State::ConfigSelect);
      return;
    case State::ConfigSelect:
      if (command(kParameters[configIndex_].address)) advance(State::ConfigRead, 5);
      return;
    case State::ConfigRead:
      if (!readConfigValue(value)) return;
      oldConfig_[configIndex_] = value;
      newConfig_[configIndex_] = kParameters[configIndex_].value |
                                 (value & kParameters[configIndex_].preserveMask);
      if (++configIndex_ < kParameterCount) {
        advance(State::ConfigSelect);
        return;
      }
      resetCapacity_ = oldConfig_[7] != kDesignCapacityMah;
      {
        bool changed = resetCapacity_;
        for (uint8_t i = 1; i < kParameterCount; ++i)
          changed = changed || oldConfig_[i] != newConfig_[i];
        if (changed || inConfig_) advance(State::EnterConfig);
        else advance(State::Seal);
      }
      return;
    case State::EnterConfig:
      if (!command(0x0090)) return;
      inConfig_ = true;
      timeoutAtMs_ = now + kStateTimeoutMs;
      advance(State::WaitConfig, kConfigWaitMs);
      return;
    case State::WaitConfig:
      if (!readWord(0x3A, value)) return;
      if (value & kConfigUpdate) {
        configIndex_ = 0;
        nextConfigWrite();
      } else if (static_cast<int32_t>(now - timeoutAtMs_) >= 0) fail("config_enter_timeout");
      else advance(State::WaitConfig, 100);
      return;
    case State::ConfigWrite: {
      const RamParameter& parameter = kParameters[configIndex_];
      value = newConfig_[configIndex_];
      uint8_t bytes[] = {static_cast<uint8_t>(parameter.address),
                         static_cast<uint8_t>(parameter.address >> 8),
                         static_cast<uint8_t>(parameter.size == 2 ? value >> 8 : value),
                         static_cast<uint8_t>(value)};
      if (writeBytes(0x3E, bytes, parameter.size + 2)) advance(State::ConfigCommit);
      return;
    }
    case State::ConfigCommit: {
      const RamParameter& parameter = kParameters[configIndex_];
      value = newConfig_[configIndex_];
      const uint8_t sum = static_cast<uint8_t>(parameter.address + (parameter.address >> 8) +
                           (value & 0xFF) + (parameter.size == 2 ? value >> 8 : 0));
      const uint8_t bytes[] = {static_cast<uint8_t>(0xFF - sum),
                               static_cast<uint8_t>(parameter.size + 4)};
      // 校验和、长度必须在同一笔写入中提交，不能拆成两个命令。
      if (writeBytes(0x60, bytes, sizeof(bytes))) advance(State::VerifySelect, 10);
      return;
    }
    case State::VerifySelect:
      if (command(kParameters[configIndex_].address)) advance(State::VerifyRead, 5);
      return;
    case State::VerifyRead:
      if (!readConfigValue(value)) return;
      if (value != newConfig_[configIndex_]) {
        fail("ram_readback_mismatch");
        return;
      }
      ++configIndex_;
      nextConfigWrite();
      return;
    case State::ExitConfig:
      // 容量迁移时重新初始化；只恢复中断配置时保留当前计量状态。
      if (!command(resetCapacity_ ? 0x0091 : 0x0092)) return;
      timeoutAtMs_ = now + kStateTimeoutMs;
      advance(State::WaitExit, kConfigWaitMs);
      return;
    case State::WaitExit:
      if (!readWord(0x3A, value)) return;
      if (!(value & kConfigUpdate)) {
        inConfig_ = false;
        advance(State::Seal);
      } else if (static_cast<int32_t>(now - timeoutAtMs_) >= 0) fail("config_exit_timeout");
      else advance(State::WaitExit, 100);
      return;
    case State::Seal:
      if (command(0x0030)) advance(State::CheckSeal, 10);
      return;
    case State::CheckSeal:
      if (!readWord(0x3A, value)) return;
      if ((value & kSecurityMask) != kSealed) {
        fail("seal_failed");
        return;
      }
      accessOpened_ = false;
      advance(State::BatteryPresence);
      return;
    case State::BatteryPresence:
      if (!readWord(0x0A, value)) return;
      timeoutAtMs_ = now + kStateTimeoutMs;
      advance((value & kBatteryPresent) ? State::WaitReady : State::InsertBattery);
      return;
    case State::InsertBattery:
      if (command(0x000D)) advance(State::WaitReady, kConfigWaitMs);
      return;
    case State::WaitReady:
      if (!readWord(0x3A, value)) return;
      if ((value & (kInitComplete | kConfigUpdate)) == kInitComplete) startSample();
      else if (static_cast<int32_t>(now - timeoutAtMs_) >= 0) fail("gauge_init_timeout");
      else advance(State::WaitReady, 100);
      return;
    case State::Idle:
      if (now - lastPollAtMs_ >= kPollIntervalMs ||
          (interruptCount_ != handledInterruptCount_ && now - lastPollAtMs_ >= kMinimumPollIntervalMs))
        startSample();
      return;
    case State::Sample:
      if (now - sampleStartedAtMs_ > 1500) {
        fail("sample_deadline_exceeded");
        return;
      }
      if (!readWord(kSampleRegisters[sampleIndex_], value)) return;
      switch (sampleIndex_) {
        case 0:
          if ((value & (kInitComplete | kConfigUpdate)) != kInitComplete) {
            fail("gauge_reset_or_config_active");
            return;
          }
          break;
        case 1:
          if (value != kDesignCapacityMah) {
            fail("gauge_configuration_lost");
            return;
          }
          break;
        case 2: pending_.temperatureCentiC = static_cast<int32_t>(value) * 10 - 27315; break;
        case 3: pending_.voltageMv = value; break;
        case 4: pending_.batteryStatus = value; break;
        case 5: pending_.currentMa = static_cast<int16_t>(value); break;
        case 6: pending_.remainingCapacityMah = value; break;
        case 7: pending_.fullChargeCapacityMah = value; break;
        case 8: pending_.averageCurrentMa = static_cast<int16_t>(value); break;
        case 9: pending_.stateOfChargePercent = value; break;
        case 10: pending_.stateOfHealthPercent = value; break;
      }
      if (++sampleIndex_ == sizeof(kSampleRegisters)) finishSample();
      else advance(State::Sample);
      return;
    case State::CleanupExit:
      command(0x0092);  // 尽力退出配置；通信断开时保持 ADC，等待下一次重试。
      inConfig_ = false;
      advance(State::CleanupSeal, kConfigWaitMs);
      return;
    case State::CleanupSeal:
      if (accessOpened_) command(0x0030);
      accessOpened_ = false;
      advance(State::Retry, kRetryIntervalMs);
      return;
    case State::Retry:
      advance(State::WakeLow);
      return;
  }
}

Bq27220Snapshot Bq27220::snapshot() const {
  Bq27220Snapshot result = data_;
  result.valid = data_.valid && static_cast<uint32_t>(millis()) - data_.sampledAtMs <= kStaleAfterMs;
  result.interruptCount = interruptCount_;
  return result;
}

const char* Bq27220::lastError() const {
  if (data_.valid && !snapshot().valid) return "sample_stale";
  return error_;
}
}  // namespace tongdou
#endif
