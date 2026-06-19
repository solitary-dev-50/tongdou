#include "voice/VoiceCommandMapper.h"

namespace tongdou {

bool VoiceCommandMapper::map(const VoiceCommandFrame& frame, ScenarioEvent& event) const {
  switch (static_cast<VoiceCommandCode>(static_cast<uint8_t>(frame.command))) {
    case VoiceCommandCode::WakeGreeting:
      event = {ScenarioEventType::BootCompleted, 0};
      return true;
    case VoiceCommandCode::QuietModeRequested:
      event = {ScenarioEventType::QuietModeRequested, 0};
      return true;
    case VoiceCommandCode::ReminderSnoozed:
      event = {ScenarioEventType::ReminderSnoozed, 0};
      return true;
    case VoiceCommandCode::ReminderConfirmed:
      event = {ScenarioEventType::ReminderConfirmed, 0};
      return true;
    case VoiceCommandCode::ReminderDue:
      event = {ScenarioEventType::ReminderDue, 0};
      return true;
    case VoiceCommandCode::GuardWord:
      return false;
  }

  event = {ScenarioEventType::VoiceRecognitionFailed, 0};
  return true;
}

}  // namespace tongdou
