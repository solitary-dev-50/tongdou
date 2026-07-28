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
};

enum class TongDouVoiceTtsState : uint8_t {
  None,
  Start,
  SentenceStart,
  Stop,
};

struct TongDouVoiceIncomingMessage {
  TongDouVoiceMessageType type = TongDouVoiceMessageType::Unknown;
  TongDouVoiceTtsState ttsState = TongDouVoiceTtsState::None;
  String sessionId;
  String text;
  String emotion;
  bool hasAudioParams = false;
  TongDouVoiceAudioParams audio;
};

class TongDouVoiceProtocol {
 public:
  String buildHello(const TongDouVoiceHelloConfig& config) const;
  String buildListenStart(const String& sessionId, const char* mode = "manual") const;
  String buildListenStop(const String& sessionId, const char* mode = "manual") const;
  String buildListenDetect(const String& sessionId, const String& text,
                           const char* mode = "manual") const;
  String buildAbort(const String& sessionId) const;

  bool parseTextMessage(const String& payload, TongDouVoiceIncomingMessage& message,
                        String& error) const;

 private:
  String buildListen(const String& sessionId, const char* state, const char* mode,
                     const String* text) const;
};

const char* tongDouVoiceMessageTypeName(TongDouVoiceMessageType type);
const char* tongDouVoiceTtsStateName(TongDouVoiceTtsState state);

}  // namespace tongdou
