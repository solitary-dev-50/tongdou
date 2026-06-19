#pragma once

#include <Arduino.h>

#include "mcp/McpToolRegistry.h"
#include "reminder/ReminderStore.h"
#include "system/TimeService.h"

namespace tongdou {

class McpServer {
 public:
  McpServer(TimeService& timeService, ReminderStore& reminderStore);

  void begin();
  String handlePayloadText(const String& payloadText);

 private:
  void addTools();
  std::string handlePayload(JsonObjectConst payload);
  std::string replyResult(const std::string& idJson, const std::string& resultJson) const;
  std::string replyError(const std::string& idJson, const std::string& message) const;
  std::string idJson(JsonObjectConst payload) const;
  std::string serializeVariant(JsonVariantConst value) const;
  std::string remindersJson() const;
  std::string statusJson() const;

  TimeService& timeService_;
  ReminderStore& reminderStore_;
  McpToolRegistry registry_;
  bool initialized_ = false;
};

}  // namespace tongdou
