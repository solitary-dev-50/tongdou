#include "motion/GyroMotionController.h"

namespace tongdou {
namespace {

constexpr unsigned long kBiasMs = 180;
constexpr int32_t kYawDeadbandMdeg = 1400;
constexpr int32_t kStraightKpDivisor = 180;
constexpr int32_t kStraightKdDivisor = 900;
constexpr uint8_t kMinDuty = 120;
constexpr uint8_t kMaxDuty = 230;
constexpr uint8_t kReturnDuty = 175;

uint8_t clampDuty(int32_t value) {
  return static_cast<uint8_t>(constrain(value, kMinDuty, kMaxDuty));
}

int32_t abs32(int32_t value) {
  return value < 0 ? -value : value;
}

}  // namespace

GyroMotionController::GyroMotionController(MotorDriver& motors, ImuSensor& imu)
    : motors_(motors), imu_(imu) {}

void GyroMotionController::begin() {
  stop();
}

void GyroMotionController::start(MotionSequence sequence) {
  stop();
  sequence_ = sequence;
  if (sequence_.frames == nullptr || sequence_.count == 0) {
    return;
  }

  if (motors_.faultActive()) {
    Serial.println(F("gyro motion skipped reason=motor_fault"));
    return;
  }

  if (!imu_.ready()) {
    Serial.println(F("gyro motion skipped reason=imu_not_ready"));
    return;
  }

  frameIndex_ = 0;
  lastGyroMs_ = 0;
  biasSumZ_ = 0;
  biasSamples_ = 0;
  biasZ_ = 0;
  yawMdeg_ = 0;
  latestYawRateMdegPerSec_ = 0;
  currentDuty_ = 0;
  gyroOk_ = true;
  phase_ = Phase::Bias;
  phaseStartedMs_ = millis();
  Serial.println(F("gyro motion begin axis=Z"));
}

void GyroMotionController::update() {
  if (phase_ == Phase::Idle) {
    return;
  }

  if (motors_.faultActive()) {
    finish(F("motor_fault"));
    return;
  }

  const unsigned long now = millis();
  updateGyro(now);
  if (!gyroOk_) {
    finish(F("gyro_read_failed"));
    return;
  }

  if (phase_ == Phase::Bias) {
    if (now - phaseStartedMs_ >= kBiasMs) {
      finishBias(now);
      startFrame(now);
    }
    return;
  }

  if (phase_ == Phase::Return) {
    if (absYaw() <= kYawDeadbandMdeg) {
      finish(F("returned"));
      return;
    }
    if (now - returnStartedMs_ >= sequence_.maxReturnMs) {
      finish(F("return_timeout"));
      return;
    }
    driveReturn();
    return;
  }

  if (phase_ != Phase::Frame) {
    return;
  }

  if (now - phaseStartedMs_ >= frameDurationMs_) {
    ++frameIndex_;
    if (frameIndex_ >= sequence_.count) {
      startReturn(now);
      return;
    }
    startFrame(now);
    return;
  }

  driveCurrentFrame();
}

void GyroMotionController::stop() {
  if (phase_ != Phase::Idle) {
    Serial.println(F("gyro motion stop"));
  }
  phase_ = Phase::Idle;
  motors_.stop();
}

bool GyroMotionController::running() const {
  return phase_ != Phase::Idle;
}

void GyroMotionController::updateGyro(unsigned long now) {
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
  latestYawRateMdegPerSec_ = yawRate;
  yawMdeg_ += static_cast<int32_t>(static_cast<int64_t>(yawRate) * dtMs / 1000);
}

void GyroMotionController::finishBias(unsigned long now) {
  if (biasSamples_ > 0) {
    biasZ_ = static_cast<int32_t>(biasSumZ_ / biasSamples_);
  }
  lastGyroMs_ = now;
  Serial.print(F("gyro motion bias_z="));
  Serial.print(biasZ_);
  Serial.print(F(" samples="));
  Serial.println(biasSamples_);
}

void GyroMotionController::startFrame(unsigned long now) {
  const MotionFrame& frame = sequence_.frames[frameIndex_];
  phase_ = Phase::Frame;
  phaseStartedMs_ = now;
  frameDurationMs_ = frame.durationMs;
  currentDuty_ = frame.duty;
  driveCurrentFrame();
}

void GyroMotionController::driveCurrentFrame() {
  const MotionFrame& frame = sequence_.frames[frameIndex_];
  if (frame.left == WheelDrive::Stop && frame.right == WheelDrive::Stop) {
    motors_.stop();
    return;
  }

  if (!currentFrameNeedsYawHold()) {
    if (sequence_.calibratedDutyPercent > 0) {
      motors_.driveAtPercent(frame.left, frame.right, sequence_.calibratedDutyPercent);
    } else {
      motors_.drive(frame.left, frame.right, currentDuty_);
    }
    return;
  }

  const int32_t correction =
      (yawMdeg_ / kStraightKpDivisor) +
      (latestYawRateMdegPerSec_ / kStraightKdDivisor);
  const int32_t leftDuty = static_cast<int32_t>(currentDuty_) - correction;
  const int32_t rightDuty = static_cast<int32_t>(currentDuty_) + correction;
  motors_.drive(frame.left, frame.right, clampDuty(leftDuty), clampDuty(rightDuty));
}

void GyroMotionController::startReturn(unsigned long now) {
  if (absYaw() <= kYawDeadbandMdeg) {
    finish(F("done"));
    return;
  }

  phase_ = Phase::Return;
  returnStartedMs_ = now;
  Serial.print(F("gyro motion return begin yaw_mdeg="));
  Serial.println(yawMdeg_);
  driveReturn();
}

void GyroMotionController::driveReturn() {
  const WheelDrive left = yawMdeg_ > 0 ? WheelDrive::Reverse : WheelDrive::Forward;
  const WheelDrive right = yawMdeg_ > 0 ? WheelDrive::Forward : WheelDrive::Reverse;
  if (sequence_.calibratedDutyPercent > 0) {
    motors_.driveAtPercent(left, right, sequence_.calibratedDutyPercent);
  } else {
    motors_.drive(left, right, kReturnDuty);
  }
}

void GyroMotionController::finish(const __FlashStringHelper* reason) {
  motors_.stop();
  Serial.print(F("gyro motion done reason="));
  Serial.print(reason);
  Serial.print(F(" yaw_mdeg="));
  Serial.println(yawMdeg_);
  phase_ = Phase::Idle;
}

bool GyroMotionController::currentFrameNeedsYawHold() const {
  const MotionFrame& frame = sequence_.frames[frameIndex_];
  return (frame.left == WheelDrive::Forward && frame.right == WheelDrive::Forward) ||
         (frame.left == WheelDrive::Reverse && frame.right == WheelDrive::Reverse);
}

int32_t GyroMotionController::absYaw() const {
  return abs32(yawMdeg_);
}

}  // namespace tongdou
