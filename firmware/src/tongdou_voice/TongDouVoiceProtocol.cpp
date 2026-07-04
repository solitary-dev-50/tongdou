#include "tongdou_voice/TongDouVoiceProtocol.h"

#include <ArduinoJson.h>

namespace tongdou {
namespace {

String serialize(JsonDocument& doc) {
  String output;
  serializeJson(doc, output);
  return output;
}

TongDouVoiceMessageType parseType(const char* type) {
  if (strcmp(type, "hello") == 0) {
    return TongDouVoiceMessageType::Hello;
  }
  if (strcmp(type, "tts") == 0) {
    return TongDouVoiceMessageType::Tts;
  }
  if (strcmp(type, "stt") == 0) {
    return TongDouVoiceMessageType::Stt;
  }
  if (strcmp(type, "llm") == 0) {
    return TongDouVoiceMessageType::Llm;
  }
  if (strcmp(type, "mcp") == 0) {
    return TongDouVoiceMessageType::Mcp;
  }
  if (strcmp(type, "server") == 0) {
    return TongDouVoiceMessageType::Server;
  }
  if (strcmp(type, "ping") == 0) {
    return TongDouVoiceMessageType::Ping;
  }
  return TongDouVoiceMessageType::Unknown;
}

TongDouVoiceTtsState parseTtsState(const char* state) {
  if (strcmp(state, "start") == 0) {
    return TongDouVoiceTtsState::Start;
  }
  if (strcmp(state, "sentence_start") == 0) {
    return TongDouVoiceTtsState::SentenceStart;
  }
  if (strcmp(state, "stop") == 0) {
    return TongDouVoiceTtsState::Stop;
  }
  return TongDouVoiceTtsState::None;
}

void writeAudioParams(JsonObject target, const TongDouVoiceAudioParams& audio) {
  target["format"] = audio.format;
  target["sample_rate"] = audio.sampleRate;
  target["channels"] = audio.channels;
  target["frame_duration"] = audio.frameDurationMs;
}

void readAudioParams(JsonObjectConst source, TongDouVoiceAudioParams& audio) {
  audio.format = source["format"] | audio.format;
  audio.sampleRate = source["sample_rate"] | audio.sampleRate;
  audio.channels = source["channels"] | audio.channels;
  audio.frameDurationMs = source["frame_duration"] | audio.frameDurationMs;
}

}  // namespace

String TongDouVoiceProtocol::buildHello(const TongDouVoiceHelloConfig& config) const {
  JsonDocument doc;
  doc["type"] = "hello";
  doc["version"] = config.version;
  doc["transport"] = config.transport;
  doc["features"]["mcp"] = config.mcpEnabled;
  writeAudioParams(doc["audio_params"].to<JsonObject>(), config.audio);
  return serialize(doc);
}

String TongDouVoiceProtocol::buildListenStart(const String& sessionId,
                                              const char* mode) const {
  return buildListen(sessionId, "start", mode, nullptr);
}

String TongDouVoiceProtocol::buildListenStop(const String& sessionId,
                                             const char* mode) const {
  return buildListen(sessionId, "stop", mode, nullptr);
}

String TongDouVoiceProtocol::buildListenDetect(const String& sessionId, const String& text,
                                               const char* mode) const {
  return buildListen(sessionId, "detect", mode, &text);
}

String TongDouVoiceProtocol::buildAbort(const String& sessionId) const {
  JsonDocument doc;
  if (sessionId.length() > 0) {
    doc["session_id"] = sessionId;
  }
  doc["type"] = "abort";
  return serialize(doc);
}

bool TongDouVoiceProtocol::parseTextMessage(const String& payload,
                                            TongDouVoiceIncomingMessage& message,
                                            String& error) const {
  JsonDocument doc;
  const DeserializationError parseError = deserializeJson(doc, payload);
  if (parseError || !doc.is<JsonObjectConst>()) {
    error = "invalid_json";
    return false;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  const char* type = root["type"] | "";
  message = {};
  message.type = parseType(type);
  message.sessionId = root["session_id"] | "";
  message.text = root["text"] | "";
  message.emotion = root["emotion"] | "";

  if (message.type == TongDouVoiceMessageType::Tts) {
    message.ttsState = parseTtsState(root["state"] | "");
  }

  JsonObjectConst audio = root["audio_params"].as<JsonObjectConst>();
  if (!audio.isNull()) {
    message.hasAudioParams = true;
    readAudioParams(audio, message.audio);
  }

  return true;
}

String TongDouVoiceProtocol::buildListen(const String& sessionId, const char* state,
                                         const char* mode, const String* text) const {
  JsonDocument doc;
  if (sessionId.length() > 0) {
    doc["session_id"] = sessionId;
  }
  doc["type"] = "listen";
  doc["state"] = state;
  doc["mode"] = mode;
  if (text != nullptr) {
    doc["text"] = *text;
  }
  return serialize(doc);
}

const char* tongDouVoiceMessageTypeName(TongDouVoiceMessageType type) {
  switch (type) {
    case TongDouVoiceMessageType::Hello:
      return "hello";
    case TongDouVoiceMessageType::Tts:
      return "tts";
    case TongDouVoiceMessageType::Stt:
      return "stt";
    case TongDouVoiceMessageType::Llm:
      return "llm";
    case TongDouVoiceMessageType::Mcp:
      return "mcp";
    case TongDouVoiceMessageType::Server:
      return "server";
    case TongDouVoiceMessageType::Ping:
      return "ping";
    case TongDouVoiceMessageType::Unknown:
    default:
      return "unknown";
  }
}

const char* tongDouVoiceTtsStateName(TongDouVoiceTtsState state) {
  switch (state) {
    case TongDouVoiceTtsState::Start:
      return "start";
    case TongDouVoiceTtsState::SentenceStart:
      return "sentence_start";
    case TongDouVoiceTtsState::Stop:
      return "stop";
    case TongDouVoiceTtsState::None:
    default:
      return "none";
  }
}

}  // namespace tongdou
