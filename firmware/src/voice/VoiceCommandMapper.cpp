#include "voice/VoiceCommandMapper.h"

namespace tongdou {

ScenarioEvent VoiceCommandMapper::map(const VoiceCommandFrame& frame) const {
  switch (static_cast<VoiceCommandCode>(frame.command)) {
    case VoiceCommandCode::WakeGreeting:
      return {ScenarioEventType::BootCompleted, 0};
    case VoiceCommandCode::ReminderDue:
      return {ScenarioEventType::ReminderDue, 0};
    case VoiceCommandCode::ReminderSnoozed:
      return {ScenarioEventType::ReminderSnoozed, 0};
    case VoiceCommandCode::ReminderConfirmed:
      return {ScenarioEventType::ReminderConfirmed, 0};
    case VoiceCommandCode::QuietModeRequested:
      return {ScenarioEventType::QuietModeRequested, 0};
    case VoiceCommandCode::RecognitionFailed:
      return {ScenarioEventType::VoiceRecognitionFailed, 0};
  }

  return {ScenarioEventType::VoiceRecognitionFailed, 0};
}

}  // namespace tongdou
