#pragma once

#include <Arduino.h>

#include "hardware/FaceDisplay.h"
#include "hardware/LedPixel.h"
#include "realtime/RealtimeVoiceService.h"

namespace tongdou {

class VoiceVisualFeedback {
 public:
  VoiceVisualFeedback(RealtimeVoiceService& voiceService,
                      FaceDisplay& faceDisplay, LedPixel& led);

  void begin();
  void update(bool higherPriorityActive = false);

 private:
  enum class VisualState : uint8_t {
    Released,
    Listening,
    Thinking,
    Speaking,
  };

  VisualState visualState(const RealtimeVoiceSnapshot& snapshot) const;
  void apply(VisualState state);
  void release();
  void settleToIdle(unsigned long nowMs);
  const char* stateName(VisualState state) const;

  RealtimeVoiceService& voiceService_;
  FaceDisplay& faceDisplay_;
  LedPixel& led_;
  VisualState appliedState_ = VisualState::Released;
  uint32_t faceRevision_ = 0;
  uint32_t ledRevision_ = 0;
  unsigned long idleSinceMs_ = 0;
  bool controlling_ = false;
};

}  // namespace tongdou
