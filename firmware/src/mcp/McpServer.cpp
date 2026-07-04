#include "mcp/McpServer.h"

#include <WiFi.h>

namespace tongdou {
namespace {

constexpr const char* kProtocolVersion = "2024-11-05";
constexpr const char* kServerName = "TongDou";
constexpr const char* kServerVersion = "tongdou-local";

std::string serializeDoc(JsonDocument& doc) {
  std::string output;
  serializeJson(doc, output);
  return output;
}

std::string toolErrorJson(const char* code, const char* message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["code"] = code;
  doc["message"] = message;
  return serializeDoc(doc);
}

}  // namespace

McpServer::McpServer(TimeService& timeService, ReminderStore& reminderStore,
                     HardwareSelfTestService& hardwareSelfTest,
                     AppCommandService& appCommandService)
    : timeService_(timeService),
      reminderStore_(reminderStore),
      hardwareSelfTest_(hardwareSelfTest),
      appCommandService_(appCommandService) {}

void McpServer::begin() {
  if (initialized_) {
    return;
  }

  addTools();
  initialized_ = true;
}

String McpServer::handlePayloadText(const String& payloadText) {
  begin();

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payloadText);
  if (error || !doc.is<JsonObjectConst>()) {
    return String(replyError("null", "Invalid JSON-RPC payload").c_str());
  }

  return String(handlePayload(doc.as<JsonObjectConst>()).c_str());
}

void McpServer::addTools() {
  const auto status = [this](const McpToolArguments&) -> McpToolResult {
    return {statusJson(), false};
  };
  const auto create = [this](const McpToolArguments& arguments) -> McpToolResult {
    return createReminder(arguments);
  };
  const auto list = [this](const McpToolArguments&) -> McpToolResult {
    return {remindersJson(), false};
  };
  const auto remove = [this](const McpToolArguments& arguments) -> McpToolResult {
    return deleteReminder(arguments);
  };
  const auto play = [this](const McpToolArguments& arguments) -> McpToolResult {
    return playScenario(arguments);
  };
  const auto personality = [this](const McpToolArguments& arguments) -> McpToolResult {
    return setPersonality(arguments);
  };
  const auto configureVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return configureVoiceBackend(arguments);
  };
  const auto connectVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return connectVoiceBackend(arguments);
  };
  const auto voiceDetectTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceDetect(arguments);
  };
  const auto voiceAbortTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceAbort(arguments);
  };
  const auto startVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return startVoiceTurn(arguments);
  };
  const auto voiceStatusTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceStatus(arguments);
  };
  const auto voiceCodecSelfTestTool =
      [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceCodecSelfTest(arguments);
  };
  const auto playVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return playVoiceResponse(arguments);
  };
  const auto failVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return failVoiceTurn(arguments);
  };

  const std::vector<McpProperty> reminderCreateProperties = {
      McpProperty::RequiredInteger("dueAt"), McpProperty::RequiredString("text")};
  const std::vector<McpProperty> reminderDeleteProperties = {
      McpProperty::RequiredInteger("id")};
  const std::vector<McpProperty> scenarioPlayProperties = {
      McpProperty::RequiredString("event")};
  const std::vector<McpProperty> personalityProperties = {
      McpProperty::RequiredString("personality")};
  const std::vector<McpProperty> voiceBackendConfigureProperties = {
      McpProperty::RequiredString("host")};
  const std::vector<McpProperty> voiceDetectProperties = {
      McpProperty::RequiredString("text")};
  const std::vector<McpProperty> voiceStartProperties = {
      McpProperty::OptionalInteger("captureMs")};
  const std::vector<McpProperty> voiceResponseProperties = {
      McpProperty::RequiredString("text")};
  const std::vector<McpProperty> voiceFailProperties = {
      McpProperty::RequiredString("reason")};

  registry_.addTool("get_status", "Get Tong Dou status.", {}, status);
  registry_.addTool("tongdou.status.get",
                    "Get Tong Dou status, including time, network, and hardware state.",
                    {}, status);

  registry_.addTool("create_reminder",
                    "Create a reminder. dueAt is Unix time in seconds.",
                    reminderCreateProperties, create);
  registry_.addTool("tongdou.reminder.create",
                    "Create a reminder. dueAt is Unix time in seconds.",
                    reminderCreateProperties, create);

  registry_.addTool("list_reminders", "List active reminders.", {}, list);
  registry_.addTool("tongdou.reminder.list", "List active reminders.", {}, list);

  registry_.addTool("delete_reminder", "Delete a reminder by id.",
                    reminderDeleteProperties, remove);
  registry_.addTool("tongdou.reminder.delete", "Delete a reminder by id.",
                    reminderDeleteProperties, remove);

  registry_.addTool("play_scenario", "Trigger a Tong Dou scenario by event name.",
                    scenarioPlayProperties, play);
  registry_.addTool("tongdou.scenario.play", "Trigger a Tong Dou scenario by event name.",
                    scenarioPlayProperties, play);

  registry_.addTool("set_personality", "Set Tong Dou personality mode.",
                    personalityProperties, personality);
  registry_.addTool("tongdou.personality.set", "Set Tong Dou personality mode.",
                    personalityProperties, personality);

  registry_.addTool("configure_voice_backend",
                    "Configure Tong Dou realtime voice backend connection.",
                    voiceBackendConfigureProperties, configureVoice);
  registry_.addTool("tongdou.voice.backend.configure",
                    "Configure Tong Dou realtime voice backend connection.",
                    voiceBackendConfigureProperties, configureVoice);

  registry_.addTool("connect_voice_backend", "Connect Tong Dou realtime voice backend.",
                    {}, connectVoice);
  registry_.addTool("tongdou.voice.backend.connect",
                    "Connect Tong Dou realtime voice backend.", {}, connectVoice);

  registry_.addTool("voice_detect", "Send text to voice backend using listen detect.",
                    voiceDetectProperties, voiceDetectTool);
  registry_.addTool("tongdou.voice.detect",
                    "Send text to voice backend using listen detect.",
                    voiceDetectProperties, voiceDetectTool);

  registry_.addTool("voice_abort", "Abort current backend speech.", {}, voiceAbortTool);
  registry_.addTool("tongdou.voice.abort", "Abort current backend speech.", {},
                    voiceAbortTool);

  registry_.addTool("start_voice_turn", "Start a realtime voice turn.",
                    voiceStartProperties,
                    startVoice);
  registry_.addTool("tongdou.voice.start", "Start a realtime voice turn.",
                    voiceStartProperties,
                    startVoice);

  registry_.addTool("voice_status", "Get realtime voice turn status.", {},
                    voiceStatusTool);
  registry_.addTool("tongdou.voice.status", "Get realtime voice turn status.", {},
                    voiceStatusTool);

  registry_.addTool("voice_codec_self_test", "Run local Opus codec self test.", {},
                    voiceCodecSelfTestTool);
  registry_.addTool("tongdou.voice.codec_self_test",
                    "Run local Opus codec self test.", {}, voiceCodecSelfTestTool);

  registry_.addTool("play_voice_response", "Play a server voice response.",
                    voiceResponseProperties, playVoice);
  registry_.addTool("tongdou.voice.play_response", "Play a server voice response.",
                    voiceResponseProperties, playVoice);

  registry_.addTool("fail_voice_turn", "Report a realtime voice failure.",
                    voiceFailProperties, failVoice);
  registry_.addTool("tongdou.voice.fail", "Report a realtime voice failure.",
                    voiceFailProperties, failVoice);
}

std::string McpServer::handlePayload(JsonObjectConst payload) {
  const char* jsonrpc = payload["jsonrpc"] | "";
  const char* method = payload["method"] | "";
  const std::string id = idJson(payload);

  if (strcmp(jsonrpc, "2.0") != 0) {
    return replyError(id, "Invalid JSON-RPC version");
  }

  if (strcmp(method, "initialize") == 0) {
    JsonDocument doc;
    doc["protocolVersion"] = kProtocolVersion;
    doc["capabilities"]["tools"].to<JsonObject>();
    doc["serverInfo"]["name"] = kServerName;
    doc["serverInfo"]["version"] = kServerVersion;
    return replyResult(id, serializeDoc(doc));
  }

  if (strcmp(method, "tools/list") == 0) {
    return replyResult(id, registry_.buildToolsListJson());
  }

  if (strcmp(method, "tools/call") == 0) {
    JsonObjectConst params = payload["params"].as<JsonObjectConst>();
    const char* name = params["name"] | "";
    if (name[0] == '\0') {
      return replyError(id, "Missing tool name");
    }

    JsonObjectConst arguments = params["arguments"].as<JsonObjectConst>();
    const McpToolResult result = registry_.callTool(name, arguments);
    return replyResult(id, mcpBuildTextResultJson(result));
  }

  return replyError(id, std::string("Unsupported method: ") + method);
}

std::string McpServer::replyResult(const std::string& idJson,
                                   const std::string& resultJson) const {
  std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
  payload += idJson.empty() ? "null" : idJson;
  payload += ",\"result\":";
  payload += resultJson;
  payload += "}";
  return payload;
}

std::string McpServer::replyError(const std::string& idJson,
                                  const std::string& message) const {
  std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
  payload += idJson.empty() ? "null" : idJson;
  payload += ",\"error\":{\"message\":\"";
  payload += mcpEscapeJsonString(message);
  payload += "\"}}";
  return payload;
}

std::string McpServer::idJson(JsonObjectConst payload) const {
  JsonVariantConst id = payload["id"];
  if (id.isNull()) {
    return "null";
  }
  return serializeVariant(id);
}

std::string McpServer::serializeVariant(JsonVariantConst value) const {
  std::string output;
  serializeJson(value, output);
  return output;
}

McpToolResult McpServer::createReminder(const McpToolArguments& arguments) {
  const uint32_t dueAt = static_cast<uint32_t>(arguments.getInt("dueAt", 0));
  const String text(arguments.getString("text").c_str());
  if (dueAt == 0 || text.length() == 0) {
    return {toolErrorJson("invalid_request", "dueAt and text are required"), true};
  }
  if (timeService_.ready() && dueAt <= static_cast<uint32_t>(timeService_.now())) {
    return {toolErrorJson("invalid_due_at", "dueAt is in the past"), true};
  }

  ReminderRecord created;
  if (!reminderStore_.add(dueAt, text, &created)) {
    return {toolErrorJson("storage_full", "reminder storage is full"), true};
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["id"] = created.id;
  doc["dueAt"] = created.dueAt;
  doc["text"] = created.text;
  return {serializeDoc(doc), false};
}

McpToolResult McpServer::deleteReminder(const McpToolArguments& arguments) {
  const uint32_t id = static_cast<uint32_t>(arguments.getInt("id", 0));
  if (id == 0 || !reminderStore_.remove(id)) {
    return {toolErrorJson("reminder_not_found", "reminder not found"), true};
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["id"] = id;
  return {serializeDoc(doc), false};
}

McpToolResult McpServer::playScenario(const McpToolArguments& arguments) {
  const String event(arguments.getString("event").c_str());
  const AppCommandResult result = appCommandService_.playScenario(event);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::setPersonality(const McpToolArguments& arguments) {
  const String personality(arguments.getString("personality").c_str());
  const AppCommandResult result = appCommandService_.setPersonality(personality);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::configureVoiceBackend(const McpToolArguments& arguments) {
  const String host(arguments.getString("host").c_str());
  const int port = arguments.getInt("port", 8000);
  const String path(arguments.getString("path", "/xiaozhi/v1/").c_str());
  const String token(arguments.getString("token").c_str());
  const String deviceId(arguments.getString("deviceId").c_str());
  const String clientId(arguments.getString("clientId", "tongdou-demo").c_str());
  const bool useTls = arguments.getInt("useTls", 0) != 0;
  const AppCommandResult result = appCommandService_.configureVoiceBackend(
      host, port, path, token, deviceId, clientId, useTls);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::connectVoiceBackend(const McpToolArguments&) {
  const AppCommandResult result = appCommandService_.connectVoiceBackend();
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceDetect(const McpToolArguments& arguments) {
  const String text(arguments.getString("text").c_str());
  const AppCommandResult result = appCommandService_.sendVoiceDetect(text);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceAbort(const McpToolArguments&) {
  const AppCommandResult result = appCommandService_.abortVoice();
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::startVoiceTurn(const McpToolArguments& arguments) {
  const int captureMs = arguments.getInt("captureMs", 2500);
  const AppCommandResult result =
      appCommandService_.startVoiceTurn(static_cast<uint16_t>(captureMs));
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceStatus(const McpToolArguments&) {
  const String status = appCommandService_.voiceStatusJson();
  return {status.c_str(), false};
}

McpToolResult McpServer::voiceCodecSelfTest(const McpToolArguments&) {
  const String result = appCommandService_.voiceCodecSelfTestJson();
  return {result.c_str(), false};
}

McpToolResult McpServer::playVoiceResponse(const McpToolArguments& arguments) {
  const String text(arguments.getString("text").c_str());
  const AppCommandResult result = appCommandService_.playVoiceResponse(text);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::failVoiceTurn(const McpToolArguments& arguments) {
  const String reason(arguments.getString("reason").c_str());
  const AppCommandResult result = appCommandService_.failVoiceTurn(reason);
  return {commandResultJson(result), !result.ok};
}

std::string McpServer::commandResultJson(const AppCommandResult& result) const {
  JsonDocument doc;
  doc["ok"] = result.ok;
  doc["code"] = result.code;
  doc["message"] = result.message;
  doc["personality"] = appCommandService_.personalityName();
  return serializeDoc(doc);
}

std::string McpServer::remindersJson() const {
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  const ReminderRecord* records = reminderStore_.records();
  for (uint8_t i = 0; i < reminderStore_.capacity(); ++i) {
    const ReminderRecord& record = records[i];
    if (!record.active) {
      continue;
    }

    JsonObject item = array.add<JsonObject>();
    item["id"] = record.id;
    item["dueAt"] = record.dueAt;
    item["completed"] = record.completed;
    item["text"] = record.text;
  }
  return serializeDoc(doc);
}

std::string McpServer::statusJson() const {
  const HardwareDiagnosticStatus hardware = hardwareSelfTest_.status();
  uint8_t activeReminders = 0;
  const ReminderRecord* records = reminderStore_.records();
  for (uint8_t i = 0; i < reminderStore_.capacity(); ++i) {
    if (records[i].active) {
      ++activeReminders;
    }
  }

  JsonDocument doc;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["timeReady"] = timeService_.ready();
  doc["now"] = timeService_.ready() ? timeService_.isoNow() : "";
  doc["systemMode"] = hardware.degraded ? "degraded" : "normal";
  doc["personality"] = appCommandService_.personalityName();
  doc["activeReminders"] = activeReminders;
  doc["reminderCapacity"] = reminderStore_.capacity();
  doc["hardware"]["face"] = hardware.faceReady;
  doc["hardware"]["led"] = hardware.ledReady;
  doc["hardware"]["audioInput"] = hardware.audioInputReady;
  doc["hardware"]["audioOutput"] = hardware.audioOutputReady;
  doc["hardware"]["degraded"] = hardware.degraded;
  doc["capabilities"]["reminders"] = hardware.remindersAvailable;
  doc["capabilities"]["web"] = hardware.webAvailable;
  doc["capabilities"]["visual"] = hardware.visualFeedbackAvailable;
  doc["capabilities"]["sound"] = hardware.soundAvailable;
  doc["capabilities"]["voiceInput"] = hardware.voiceInputAvailable;
  return serializeDoc(doc);
}

}  // namespace tongdou
