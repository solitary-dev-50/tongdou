#pragma once

#include <Arduino.h>

#include "tongdou_voice/TongDouVoiceConfig.h"

namespace tongdou {

enum class TongDouVoiceMessageType : uint8_t {
  Unknown,
  Hello,
  Tts,
  Stt,
  Llm,
  Mcp,
  Server,
  Ping,
  Goodbye,
  Reminder,
};

enum class TongDouVoiceTtsState : uint8_t {
  None,
  Start,
  SentenceStart,
  Stop,
};

enum class TongDouVoiceListenMode : uint8_t {
  Manual,
  Auto,
  Realtime,
};

enum class TongDouVoiceReminderState : uint8_t {
  None,
  Accepted,
  Failed,
};

struct TongDouVoiceIncomingMessage {
  TongDouVoiceMessageType type = TongDouVoiceMessageType::Unknown;
  TongDouVoiceTtsState ttsState = TongDouVoiceTtsState::None;
  TongDouVoiceReminderState reminderState = TongDouVoiceReminderState::None;
  uint32_t reminderId = 0;
  String sessionId;
  String text;
  String emotion;
  String reason;
  String mcpPayload;
  String localeResourceBaseUrl;
  uint16_t localeResourceFormatVersion = 0;
  bool hasAudioParams = false;
  TongDouVoiceAudioParams audio;
};

class TongDouVoiceProtocol {
 public:
  String buildHello(const TongDouVoiceHelloConfig& config) const;
  String buildListenStart(
      const String& sessionId,
      TongDouVoiceListenMode mode = TongDouVoiceListenMode::Manual) const;
  String buildListenStop(
      const String& sessionId,
      TongDouVoiceListenMode mode = TongDouVoiceListenMode::Manual) const;
  String buildListenDetect(const String& sessionId, const String& text,
                           TongDouVoiceListenMode mode =
                               TongDouVoiceListenMode::Manual) const;
  String buildAbort(const String& sessionId) const;
  String buildGoodbyeRequest(const String& sessionId,
                             const String& reason = "manual") const;
  String buildReminderAnnouncement(const String& sessionId, uint32_t reminderId,
                                   const String& text,
                                   const String& locale,
                                   const String& proactiveEvent = "") const;
  String buildMcpMessage(const String& payload) const;

  bool parseTextMessage(const String& payload, TongDouVoiceIncomingMessage& message,
                        String& error) const;

 private:
  String buildListen(const String& sessionId, const char* state,
                     TongDouVoiceListenMode mode, const String* text) const;
};

const char* tongDouVoiceMessageTypeName(TongDouVoiceMessageType type);
const char* tongDouVoiceTtsStateName(TongDouVoiceTtsState state);
const char* tongDouVoiceListenModeName(TongDouVoiceListenMode mode);
const char* tongDouVoiceReminderStateName(TongDouVoiceReminderState state);

}  // namespace tongdou
