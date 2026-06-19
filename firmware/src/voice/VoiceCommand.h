#pragma once

#include <stdint.h>

namespace tongdou {

constexpr uint8_t kVoiceCommandHeader = 0xAA;

enum class VoiceCommandCode : uint8_t {
  WakeGreeting = 0x01,
  QuietModeRequested = 0x02,
  ReminderSnoozed = 0x03,
  ReminderConfirmed = 0x04,
  ReminderDue = 0x05,
  GuardWord = 0x06,
};

struct VoiceCommandFrame {
  uint8_t command = 0;
};

}  // namespace tongdou
