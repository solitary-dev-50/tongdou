#pragma once

#include <stdint.h>

namespace tongdou {

enum class FaceAction : uint8_t {
  None,
  WakeUp,
  Smile,
  Serious,
  RollEyes,
  Sleepy,
  Wronged,
};

enum class LightAction : uint8_t {
  None,
  Off,
  SoftWhite,
  WarmWake,
  RedShortBlink,
  WeakBreath,
};

enum class MotionAction : uint8_t {
  None,
  Stop,
  Nod,
  TinyShake,
  NudgeForward,
};

enum class VoiceLine : uint8_t {
  None,
  WakeGreeting,
  SnoozeSoft,
  SnoozeTease,
  ConfirmReward,
  QuietReply,
  BreakReminder,
};

}  // namespace tongdou
