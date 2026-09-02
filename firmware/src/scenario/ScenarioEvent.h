#pragma once

#include <stdint.h>

namespace tongdou {

enum class ScenarioEventType : uint8_t {
  BootCompleted,
  UserArrived,
  UserLeft,
  UserIdleTooLong,
  HydrationReminderDue,
  ReminderDue,
  ReminderConfirmed,
  ReminderSnoozed,
  QuietModeRequested,
  LowBattery,
  ChargingStarted,
  OvertimeReminderDue,
  VoiceRecognitionFailed,
  ConversationWindowTimeout,
  DanceShowRequested,
  DanceShowGlobalRequested,
  LogoSingleTapped,
  LogoDoubleTapped,
  LogoLongPressed,
};

struct ScenarioEvent {
  ScenarioEventType type = ScenarioEventType::BootCompleted;
  uint8_t repeatCount = 0;
};

}  // namespace tongdou
