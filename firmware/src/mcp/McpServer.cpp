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

}  // namespace

McpServer::McpServer(TimeService& timeService, ReminderStore& reminderStore)
    : timeService_(timeService), reminderStore_(reminderStore) {}

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
  registry_.addTool(
      "tongdou.status.get", "Get Tong Dou status, including time and network state.", {},
      [this](const McpToolArguments&) -> McpToolResult { return {statusJson(), false}; });

  registry_.addTool(
      "tongdou.reminder.create", "Create a reminder. dueAt is Unix time in seconds.",
      {McpProperty::RequiredInteger("dueAt"), McpProperty::RequiredString("text")},
      [this](const McpToolArguments& arguments) -> McpToolResult {
        const uint32_t dueAt = static_cast<uint32_t>(arguments.getInt("dueAt", 0));
        const String text(arguments.getString("text").c_str());
        if (dueAt == 0 || text.length() == 0) {
          return {"dueAt and text are required", true};
        }
        if (timeService_.ready() && dueAt <= static_cast<uint32_t>(timeService_.now())) {
          return {"dueAt is in the past", true};
        }

        ReminderRecord created;
        if (!reminderStore_.add(dueAt, text, &created)) {
          return {"reminder storage is full", true};
        }

        JsonDocument doc;
        doc["id"] = created.id;
        doc["dueAt"] = created.dueAt;
        doc["text"] = created.text;
        return {serializeDoc(doc), false};
      });

  registry_.addTool(
      "tongdou.reminder.list", "List active reminders.", {},
      [this](const McpToolArguments&) -> McpToolResult { return {remindersJson(), false}; });

  registry_.addTool(
      "tongdou.reminder.delete", "Delete a reminder by id.",
      {McpProperty::RequiredInteger("id")},
      [this](const McpToolArguments& arguments) -> McpToolResult {
        const uint32_t id = static_cast<uint32_t>(arguments.getInt("id", 0));
        if (id == 0 || !reminderStore_.remove(id)) {
          return {"reminder not found", true};
        }
        return {"{\"ok\":true}", false};
      });
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

  return replyError(id, std::string("Method not implemented: ") + method);
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
  JsonDocument doc;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["timeReady"] = timeService_.ready();
  doc["now"] = timeService_.ready() ? timeService_.isoNow() : "";
  doc["reminderCapacity"] = reminderStore_.capacity();
  return serializeDoc(doc);
}

}  // namespace tongdou
