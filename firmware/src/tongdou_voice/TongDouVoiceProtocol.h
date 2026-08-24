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

struct TongDouVoiceIncomingMessage {
  TongDouVoiceMessageType type = TongDouVoiceMessageType::Unknown;
  TongDouVoiceTtsState ttsState = TongDouVoiceTtsState::None;
  String sessionId;
  String text;
  String emotion;
  String reason;
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

  bool parseTextMessage(const String& payload, TongDouVoiceIncomingMessage& message,
                        String& error) const;

 private:
  String buildListen(const String& sessionId, const char* state,
                     TongDouVoiceListenMode mode, const String* text) const;
};

const char* tongDouVoiceMessageTypeName(TongDouVoiceMessageType type);
const char* tongDouVoiceTtsStateName(TongDouVoiceTtsState state);
const char* tongDouVoiceListenModeName(TongDouVoiceListenMode mode);

}  // namespace tongdou
