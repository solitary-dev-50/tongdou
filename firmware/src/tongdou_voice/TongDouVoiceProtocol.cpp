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
  if (strcmp(type, "goodbye") == 0) {
    return TongDouVoiceMessageType::Goodbye;
  }
  if (strcmp(type, "tongdou_reminder") == 0) {
    return TongDouVoiceMessageType::Reminder;
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

TongDouVoiceReminderState parseReminderState(const char* state) {
  if (strcmp(state, "accepted") == 0) {
    return TongDouVoiceReminderState::Accepted;
  }
  if (strcmp(state, "failed") == 0) {
    return TongDouVoiceReminderState::Failed;
  }
  return TongDouVoiceReminderState::None;
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
  doc["features"]["goodbye"] = config.goodbyeEnabled;
  writeAudioParams(doc["audio_params"].to<JsonObject>(), config.audio);
  return serialize(doc);
}

String TongDouVoiceProtocol::buildListenStart(
    const String& sessionId, TongDouVoiceListenMode mode) const {
  return buildListen(sessionId, "start", mode, nullptr);
}

String TongDouVoiceProtocol::buildListenStop(
    const String& sessionId, TongDouVoiceListenMode mode) const {
  return buildListen(sessionId, "stop", mode, nullptr);
}

String TongDouVoiceProtocol::buildListenDetect(const String& sessionId, const String& text,
                                               TongDouVoiceListenMode mode) const {
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

String TongDouVoiceProtocol::buildGoodbyeRequest(const String& sessionId,
                                                 const String& reason) const {
  JsonDocument doc;
  if (sessionId.length() > 0) {
    doc["session_id"] = sessionId;
  }
  doc["type"] = "goodbye";
  doc["reason"] = reason;
  return serialize(doc);
}

String TongDouVoiceProtocol::buildReminderAnnouncement(
    const String& sessionId, uint32_t reminderId, const String& text,
    const String& locale) const {
  JsonDocument doc;
  if (sessionId.length() > 0) {
    doc["session_id"] = sessionId;
  }
  doc["type"] = "tongdou_reminder";
  doc["id"] = reminderId;
  doc["text"] = text;
  doc["locale"] = locale;
  return serialize(doc);
}

String TongDouVoiceProtocol::buildMcpMessage(const String& payload) const {
  JsonDocument payloadDoc;
  const DeserializationError parseError = deserializeJson(payloadDoc, payload);
  if (parseError) {
    return "";
  }

  JsonDocument doc;
  doc["type"] = "mcp";
  doc["payload"].set(payloadDoc.as<JsonVariantConst>());
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
  message.reason = root["reason"] | "";
  message.reminderId = root["id"] | 0U;

  JsonObjectConst localeResources =
      root["locale_resources"].as<JsonObjectConst>();
  if (!localeResources.isNull()) {
    message.localeResourceBaseUrl = localeResources["base_url"] | "";
    message.localeResourceFormatVersion =
        localeResources["format_version"] | 0;
  }

  if (message.type == TongDouVoiceMessageType::Mcp) {
    JsonVariantConst mcpPayload = root["payload"];
    if (mcpPayload.isNull()) {
      error = "mcp_payload_missing";
      return false;
    }
    serializeJson(mcpPayload, message.mcpPayload);
  }

  if (message.type == TongDouVoiceMessageType::Tts) {
    message.ttsState = parseTtsState(root["state"] | "");
  }
  if (message.type == TongDouVoiceMessageType::Reminder) {
    message.reminderState = parseReminderState(root["state"] | "");
  }

  JsonObjectConst audio = root["audio_params"].as<JsonObjectConst>();
  if (!audio.isNull()) {
    message.hasAudioParams = true;
    readAudioParams(audio, message.audio);
  }

  return true;
}

String TongDouVoiceProtocol::buildListen(const String& sessionId, const char* state,
                                         TongDouVoiceListenMode mode,
                                         const String* text) const {
  JsonDocument doc;
  if (sessionId.length() > 0) {
    doc["session_id"] = sessionId;
  }
  doc["type"] = "listen";
  doc["state"] = state;
  doc["mode"] = tongDouVoiceListenModeName(mode);
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
    case TongDouVoiceMessageType::Goodbye:
      return "goodbye";
    case TongDouVoiceMessageType::Reminder:
      return "tongdou_reminder";
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

const char* tongDouVoiceListenModeName(TongDouVoiceListenMode mode) {
  switch (mode) {
    case TongDouVoiceListenMode::Auto:
      return "auto";
    case TongDouVoiceListenMode::Realtime:
      return "realtime";
    case TongDouVoiceListenMode::Manual:
    default:
      return "manual";
  }
}

const char* tongDouVoiceReminderStateName(TongDouVoiceReminderState state) {
  switch (state) {
    case TongDouVoiceReminderState::Accepted:
      return "accepted";
    case TongDouVoiceReminderState::Failed:
      return "failed";
    case TongDouVoiceReminderState::None:
    default:
      return "none";
  }
}

}  // namespace tongdou
