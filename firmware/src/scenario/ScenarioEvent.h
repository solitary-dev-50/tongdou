#pragma once

#include <stdint.h>

namespace tongdou {

enum class ScenarioEventType : uint8_t {
  BootCompleted,
  UserArrived,
  UserLeft,
  UserIdleTooLong,
  ReminderDue,
  ReminderConfirmed,
  ReminderSnoozed,
  QuietModeRequested,
  LowBattery,
  ChargingStarted,
  OvertimeReminderDue,
  VoiceRecognitionFailed,
  DanceShowRequested,
  DanceShowGlobalRequested,
};

struct ScenarioEvent {
  ScenarioEventType type = ScenarioEventType::BootCompleted;
  uint8_t repeatCount = 0;
};

}  // namespace tongdou
