#include "motion/GyroReturnWiggleController.h"

#include <Arduino.h>

namespace tongdou {
namespace {

constexpr unsigned long kBiasMs = 240;
constexpr unsigned long kReturnProbeMs = 90;
constexpr unsigned long kReturnMaxMs = 700;
constexpr int32_t kReturnDeadbandMdeg = 1800;
constexpr int32_t kReturnFlipMarginMdeg = 300;
constexpr uint8_t kWigglePwm1 = 200;
constexpr uint8_t kWigglePwm2 = 195;
constexpr uint8_t kReturnPwm = 180;

int32_t abs32(int32_t value) {
  return value < 0 ? -value : value;
}

}  // namespace

GyroReturnWiggleController::GyroReturnWiggleController(MotorDriver& motors,
                                                       ImuSensor& imu)
    : motors_(motors), imu_(imu) {}

void GyroReturnWiggleController::start() {
  stop();
  if (!imu_.ready()) {
    Serial.println(F("gyro wiggle failed reason=imu_not_ready"));
    return;
  }

  biasSumZ_ = 0;
  biasSamples_ = 0;
  biasZ_ = 0;
  yawAngleMdeg_ = 0;
  returnDirection_ = 0;
  returnProbeStartAbsMdeg_ = 0;
  gyroOk_ = true;
  lastGyroMs_ = 0;
  Serial.println(F("gyro wiggle begin axis=Z"));
  startPhase(Phase::Bias, kBiasMs);
}

void GyroReturnWiggleController::update() {
  if (phase_ == Phase::Idle) {
    return;
  }

  const unsigned long now = millis();
  updateGyro(now);
  if (!gyroOk_) {
    finish(F("gyro_read_failed"));
    return;
  }

  if (phase_ == Phase::ReturnProbe || phase_ == Phase::ReturnDrive) {
    updateReturn(now);
    return;
  }

  if (now - phaseStartedMs_ < phaseDurationMs_) {
    return;
  }

  switch (phase_) {
    case Phase::Bias:
      finishBias();
      startDrivePhase(Phase::WiggleLeft1, 210, WheelDrive::Forward,
                      WheelDrive::Reverse, kWigglePwm1);
      break;
    case Phase::WiggleLeft1:
      startPhase(Phase::Pause1, 120);
      break;
    case Phase::Pause1:
      startDrivePhase(Phase::WiggleRight1, 210, WheelDrive::Reverse,
                      WheelDrive::Forward, kWigglePwm1);
      break;
    case Phase::WiggleRight1:
      startPhase(Phase::Pause2, 120);
      break;
    case Phase::Pause2:
      startDrivePhase(Phase::WiggleLeft2, 170, WheelDrive::Forward,
                      WheelDrive::Reverse, kWigglePwm2);
      break;
    case Phase::WiggleLeft2:
      startPhase(Phase::Pause3, 100);
      break;
    case Phase::Pause3:
      startDrivePhase(Phase::WiggleRight2, 170, WheelDrive::Reverse,
                      WheelDrive::Forward, kWigglePwm2);
      break;
    case Phase::WiggleRight2:
      startPhase(Phase::Settle, 120);
      break;
    case Phase::Settle:
      enterReturn(now);
      break;
    case Phase::Idle:
    case Phase::ReturnProbe:
    case Phase::ReturnDrive:
      break;
  }
}

void GyroReturnWiggleController::stop() {
  if (phase_ != Phase::Idle) {
    Serial.println(F("gyro wiggle stop"));
  }
  phase_ = Phase::Idle;
  motors_.stop();
}

bool GyroReturnWiggleController::running() const {
  return phase_ != Phase::Idle;
}

void GyroReturnWiggleController::startPhase(Phase phase,
                                            unsigned long durationMs) {
  phase_ = phase;
  phaseStartedMs_ = millis();
  phaseDurationMs_ = durationMs;
  motors_.stop();
}

void GyroReturnWiggleController::startDrivePhase(Phase phase,
                                                 unsigned long durationMs,
                                                 WheelDrive left,
                                                 WheelDrive right,
                                                 uint8_t duty) {
  phase_ = phase;
  phaseStartedMs_ = millis();
  phaseDurationMs_ = durationMs;
  motors_.drive(left, right, 0, duty, duty);
  Serial.print(F("gyro wiggle drive phase="));
  Serial.print(static_cast<int>(phase));
  Serial.print(F(" duty="));
  Serial.print(duty);
  Serial.print(F(" angle_mdeg="));
  Serial.println(yawAngleMdeg_);
}

void GyroReturnWiggleController::updateGyro(unsigned long now) {
  const ImuGyroSnapshot gyro = imu_.readGyro();
  if (!gyro.ready) {
    gyroOk_ = false;
    return;
  }

  if (phase_ == Phase::Bias) {
    biasSumZ_ += gyro.zMdegPerSec;
    ++biasSamples_;
    return;
  }

  if (lastGyroMs_ == 0) {
    lastGyroMs_ = now;
    return;
  }

  unsigned long dtMs = now - lastGyroMs_;
  dtMs = constrain(dtMs, 1UL, 80UL);
  lastGyroMs_ = now;
  const int32_t yawRate = gyro.zMdegPerSec - biasZ_;
  yawAngleMdeg_ +=
      static_cast<int32_t>(static_cast<int64_t>(yawRate) * dtMs / 1000);
}

void GyroReturnWiggleController::finishBias() {
  if (biasSamples_ > 0) {
    biasZ_ = static_cast<int32_t>(biasSumZ_ / biasSamples_);
  }
  lastGyroMs_ = millis();
  Serial.print(F("gyro wiggle bias_z="));
  Serial.print(biasZ_);
  Serial.print(F(" samples="));
  Serial.println(biasSamples_);
}

void GyroReturnWiggleController::enterReturn(unsigned long now) {
  motors_.stop();
  returnStartedMs_ = now;
  returnDirection_ = yawAngleMdeg_ > 0 ? -1 : 1;
  returnProbeStartAbsMdeg_ = absAngle();
  returnProbeStartedMs_ = now;
  Serial.print(F("gyro wiggle return begin angle_mdeg="));
  Serial.print(yawAngleMdeg_);
  Serial.print(F(" direction="));
  Serial.println(returnDirection_);

  if (returnProbeStartAbsMdeg_ <= kReturnDeadbandMdeg) {
    finish(F("already_centered"));
    return;
  }

  phase_ = Phase::ReturnProbe;
  driveReturn(returnDirection_);
}

void GyroReturnWiggleController::updateReturn(unsigned long now) {
  if (absAngle() <= kReturnDeadbandMdeg) {
    finish(F("centered"));
    return;
  }

  if (now - returnStartedMs_ >= kReturnMaxMs) {
    finish(F("timeout"));
    return;
  }

  if (phase_ == Phase::ReturnProbe &&
      now - returnProbeStartedMs_ >= kReturnProbeMs) {
    if (absAngle() > returnProbeStartAbsMdeg_ + kReturnFlipMarginMdeg) {
      returnDirection_ = -returnDirection_;
      Serial.print(F("gyro wiggle return flip direction="));
      Serial.println(returnDirection_);
    }
    phase_ = Phase::ReturnDrive;
    driveReturn(returnDirection_);
  }
}

void GyroReturnWiggleController::driveReturn(int8_t direction) {
  if (direction >= 0) {
    motors_.drive(WheelDrive::Forward, WheelDrive::Reverse, 0, kReturnPwm,
                  kReturnPwm);
  } else {
    motors_.drive(WheelDrive::Reverse, WheelDrive::Forward, 0, kReturnPwm,
                  kReturnPwm);
  }
}

void GyroReturnWiggleController::finish(const __FlashStringHelper* reason) {
  motors_.stop();
  Serial.print(F("gyro wiggle done reason="));
  Serial.print(reason);
  Serial.print(F(" angle_mdeg="));
  Serial.println(yawAngleMdeg_);
  phase_ = Phase::Idle;
}

int32_t GyroReturnWiggleController::absAngle() const {
  return abs32(yawAngleMdeg_);
}

}  // namespace tongdou
