#include "realtime/VoiceVisualFeedback.h"

namespace tongdou {
namespace {

constexpr unsigned long kIdleSettleMs = 250;
constexpr RgbColor kListeningColor = {0, 14, 18};
constexpr RgbColor kThinkingColor = {5, 3, 14};
constexpr RgbColor kSpeakingColor = {16, 9, 3};

}  // namespace

VoiceVisualFeedback::VoiceVisualFeedback(RealtimeVoiceService& voiceService,
                                         FaceDisplay& faceDisplay,
                                         LedPixel& led)
    : voiceService_(voiceService), faceDisplay_(faceDisplay), led_(led) {}

void VoiceVisualFeedback::begin() {
  release();
}

void VoiceVisualFeedback::update(bool higherPriorityActive) {
  const RealtimeVoiceSnapshot snapshot = voiceService_.snapshot();
  if (higherPriorityActive || snapshot.reminderAnnouncementActive ||
      snapshot.state == RealtimeVoiceState::Failed) {
    release();
    return;
  }

  if (snapshot.state == RealtimeVoiceState::Idle) {
    settleToIdle(millis());
    return;
  }

  idleSinceMs_ = 0;
  const VisualState requested = visualState(snapshot);
  const bool displayWasChanged =
      controlling_ && (faceDisplay_.revision() != faceRevision_ ||
                       led_.revision() != ledRevision_);
  if (!controlling_ || requested != appliedState_ || displayWasChanged) {
    apply(requested);
  }
}

VoiceVisualFeedback::VisualState VoiceVisualFeedback::visualState(
    const RealtimeVoiceSnapshot& snapshot) const {
  if (snapshot.state == RealtimeVoiceState::Listening) {
    return VisualState::Listening;
  }
  if (snapshot.state == RealtimeVoiceState::Ending ||
      snapshot.speechOutputActive) {
    return VisualState::Speaking;
  }
  return VisualState::Thinking;
}

void VoiceVisualFeedback::apply(VisualState state) {
  switch (state) {
    case VisualState::Listening:
      faceDisplay_.show(FaceExpression::Listening);
      led_.show(kListeningColor);
      break;
    case VisualState::Thinking:
      faceDisplay_.show(FaceExpression::Serious);
      led_.show(kThinkingColor);
      break;
    case VisualState::Speaking:
      faceDisplay_.show(FaceExpression::Smile);
      led_.show(kSpeakingColor);
      break;
    case VisualState::Released:
    default:
      return;
  }

  controlling_ = true;
  appliedState_ = state;
  faceRevision_ = faceDisplay_.revision();
  ledRevision_ = led_.revision();
  Serial.print(F("voice visual state="));
  Serial.println(stateName(state));
}

void VoiceVisualFeedback::release() {
  controlling_ = false;
  appliedState_ = VisualState::Released;
  idleSinceMs_ = 0;
  faceRevision_ = faceDisplay_.revision();
  ledRevision_ = led_.revision();
}

void VoiceVisualFeedback::settleToIdle(unsigned long nowMs) {
  if (!controlling_) {
    idleSinceMs_ = 0;
    return;
  }
  if (idleSinceMs_ == 0) {
    idleSinceMs_ = nowMs;
    return;
  }
  if (static_cast<long>(nowMs - idleSinceMs_) <
      static_cast<long>(kIdleSettleMs)) {
    return;
  }

  faceDisplay_.show(FaceExpression::Sleep);
  led_.off();
  Serial.println(F("voice visual state=idle"));
  release();
}

const char* VoiceVisualFeedback::stateName(VisualState state) const {
  switch (state) {
    case VisualState::Listening:
      return "listening";
    case VisualState::Thinking:
      return "thinking";
    case VisualState::Speaking:
      return "speaking";
    case VisualState::Released:
    default:
      return "released";
  }
}

}  // namespace tongdou
