#pragma once

#include <Arduino.h>

namespace tongdou {

enum class LogoTouchEvent : uint8_t {
  None,
  SingleTap,
  DoubleTap,
  LongPress,
};

class LogoTouchInput {
 public:
  void begin();
  void update();

  LogoTouchEvent consumeEvent();

  bool ready() const;
  uint32_t raw() const;
  uint32_t baseline() const;
  int32_t delta() const;
  uint32_t threshold() const;

 private:
  void finishCalibration(unsigned long now);
  void updateTapState(bool touched, unsigned long now);
  void emitSingleTapIfWindowExpired(unsigned long now);

  bool ready_ = false;
  bool calibrated_ = false;
  bool wasTouched_ = false;
  bool touchCandidate_ = false;
  bool tapPending_ = false;
  bool longPressSent_ = false;
  uint32_t raw_ = 0;
  uint32_t baseline_ = 0;
  uint64_t calibrationSum_ = 0;
  uint16_t calibrationSamples_ = 0;
  unsigned long beginMs_ = 0;
  unsigned long nextSampleMs_ = 0;
  unsigned long touchCandidateStartedMs_ = 0;
  unsigned long touchStartedMs_ = 0;
  unsigned long firstTapReleasedMs_ = 0;
  LogoTouchEvent pendingEvent_ = LogoTouchEvent::None;
};

}  // namespace tongdou
