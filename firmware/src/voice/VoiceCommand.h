#pragma once

#include <stdint.h>

namespace tongdou {

constexpr uint8_t kVoiceCommandHeader = 0xAA;
constexpr uint8_t kCiProtocolHeadLow = 0xA5;
constexpr uint8_t kCiProtocolHeadHigh = 0xFC;
constexpr uint8_t kCiProtocolTail = 0xFB;
constexpr uint8_t kCiProtocolTypeCommandUp = 0xA0;
constexpr uint8_t kCiProtocolTypeCommandDown = 0xA1;
constexpr uint8_t kCiProtocolCommandAsrResult = 0x91;
constexpr uint8_t kCiProtocolCommandPlayVoice = 0x92;
constexpr uint8_t kCiProtocolPlayStart = 0x80;
constexpr uint8_t kCiProtocolPlayByVoiceId = 0x90;
constexpr uint8_t kCiAsrResultLength = 7;
constexpr uint8_t kCiPlayVoiceLength = 6;
constexpr uint8_t kCiMaxDataLength = 20;

enum class VoiceCommandCode : uint8_t {
  WakeGreeting = 0x01,
  QuietModeRequested = 0x02,
  ReminderSnoozed = 0x03,
  ReminderConfirmed = 0x04,
  ReminderDue = 0x05,
  GuardWord = 0x06,
};

struct VoiceCommandFrame {
  uint32_t command = 0;
};

struct VoicePlaybackFrame {
  static constexpr uint8_t kMaxLength = 16;

  uint8_t bytes[kMaxLength] = {};
  uint8_t length = 0;
};

}  // namespace tongdou
