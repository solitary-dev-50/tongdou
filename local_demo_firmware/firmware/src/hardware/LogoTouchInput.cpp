#include "hardware/LogoTouchInput.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr unsigned long kSampleIntervalMs = 20;
constexpr unsigned long kCalibrationMs = 700;
constexpr unsigned long kDebounceMs = 35;
constexpr unsigned long kTapMaxMs = 700;
constexpr unsigned long kDoubleTapWindowMs = 280;
constexpr unsigned long kLongPressMs = 1100;
constexpr uint32_t kMinThreshold = 160;

uint32_t absDiff(uint32_t a, uint32_t b) {
  return a > b ? a - b : b - a;
}

}  // namespace

void LogoTouchInput::begin() {
  beginMs_ = millis();
  nextSampleMs_ = beginMs_;
  raw_ = touchRead(static_cast<uint8_t>(pins::LOGO_TOUCH));
  baseline_ = raw_;
  calibrationSum_ = 0;
  calibrationSamples_ = 0;
  calibrated_ = false;
  wasTouched_ = false;
  touchCandidate_ = false;
  tapPending_ = false;
  longPressSent_ = false;
  pendingEvent_ = LogoTouchEvent::None;
  ready_ = raw_ != 0;

  Serial.print("logo touch begin pin=");
  Serial.print(static_cast<int>(pins::LOGO_TOUCH));
  Serial.print(" raw=");
  Serial.println(raw_);
}

void LogoTouchInput::update() {
  const unsigned long now = millis();
  if (static_cast<long>(now - nextSampleMs_) < 0) {
    return;
  }
  nextSampleMs_ = now + kSampleIntervalMs;

  raw_ = touchRead(static_cast<uint8_t>(pins::LOGO_TOUCH));
  if (raw_ == 0) {
    ready_ = false;
  }

  if (!calibrated_) {
    calibrationSum_ += raw_;
    ++calibrationSamples_;
    if (now - beginMs_ >= kCalibrationMs) {
      finishCalibration(now);
    }
    return;
  }

  const bool touched = absDiff(raw_, baseline_) >= threshold();
  updateTapState(touched, now);
  emitSingleTapIfWindowExpired(now);

  if (!touched && !wasTouched_) {
    baseline_ = ((baseline_ * 31U) + raw_) / 32U;
  }
}

LogoTouchEvent LogoTouchInput::consumeEvent() {
  const LogoTouchEvent event = pendingEvent_;
  pendingEvent_ = LogoTouchEvent::None;
  return event;
}

bool LogoTouchInput::ready() const {
  return ready_;
}

uint32_t LogoTouchInput::raw() const {
  return raw_;
}

uint32_t LogoTouchInput::baseline() const {
  return baseline_;
}

int32_t LogoTouchInput::delta() const {
  return static_cast<int32_t>(raw_) - static_cast<int32_t>(baseline_);
}

uint32_t LogoTouchInput::threshold() const {
  const uint32_t adaptive = baseline_ / 64U;
  return adaptive > kMinThreshold ? adaptive : kMinThreshold;
}

void LogoTouchInput::finishCalibration(unsigned long now) {
  if (calibrationSamples_ > 0) {
    baseline_ = static_cast<uint32_t>(calibrationSum_ / calibrationSamples_);
  }
  calibrated_ = true;
  ready_ = baseline_ != 0;
  Serial.print("logo touch ready=");
  Serial.print(ready_ ? "1" : "0");
  Serial.print(" raw=");
  Serial.print(raw_);
  Serial.print(" baseline=");
  Serial.print(baseline_);
  Serial.print(" threshold=");
  Serial.print(threshold());
  Serial.print(" samples=");
  Serial.println(calibrationSamples_);
  nextSampleMs_ = now + kSampleIntervalMs;
}

void LogoTouchInput::updateTapState(bool touched, unsigned long now) {
  if (touched && !wasTouched_) {
    if (!touchCandidate_) {
      touchCandidate_ = true;
      touchCandidateStartedMs_ = now;
      return;
    }

    if (now - touchCandidateStartedMs_ < kDebounceMs) {
      return;
    }

    wasTouched_ = true;
    longPressSent_ = false;
    touchStartedMs_ = touchCandidateStartedMs_;
    Serial.print("logo touch down raw=");
    Serial.print(raw_);
    Serial.print(" baseline=");
    Serial.print(baseline_);
    Serial.print(" delta=");
    Serial.println(delta());
    return;
  }

  if (touched && wasTouched_ && !longPressSent_ &&
      now - touchStartedMs_ >= kLongPressMs) {
    longPressSent_ = true;
    tapPending_ = false;
    pendingEvent_ = LogoTouchEvent::LongPress;
    Serial.println("logo touch event=long_press");
    return;
  }

  if (!touched && touchCandidate_ && !wasTouched_) {
    touchCandidate_ = false;
    return;
  }

  if (!touched && wasTouched_) {
    wasTouched_ = false;
    touchCandidate_ = false;
    const unsigned long pressMs = now - touchStartedMs_;
    Serial.print("logo touch up press_ms=");
    Serial.print(pressMs);
    Serial.print(" raw=");
    Serial.print(raw_);
    Serial.print(" baseline=");
    Serial.print(baseline_);
    Serial.print(" delta=");
    Serial.println(delta());

    if (longPressSent_) {
      return;
    }

    if (pressMs < kDebounceMs || pressMs > kTapMaxMs) {
      return;
    }

    if (tapPending_ && now - firstTapReleasedMs_ <= kDoubleTapWindowMs) {
      tapPending_ = false;
      pendingEvent_ = LogoTouchEvent::DoubleTap;
      Serial.println("logo touch event=double_tap");
      return;
    }

    tapPending_ = true;
    firstTapReleasedMs_ = now;
  }
}

void LogoTouchInput::emitSingleTapIfWindowExpired(unsigned long now) {
  if (!tapPending_) {
    return;
  }
  if (now - firstTapReleasedMs_ <= kDoubleTapWindowMs) {
    return;
  }
  tapPending_ = false;
  pendingEvent_ = LogoTouchEvent::SingleTap;
  Serial.println("logo touch event=single_tap");
}

}  // namespace tongdou
