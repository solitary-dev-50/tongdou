#pragma once

#include <Arduino.h>

#include "app/AppCommandService.h"
#include "diagnostic/HardwareSelfTestService.h"
#include "mcp/McpToolRegistry.h"
#include "reminder/ReminderStore.h"
#include "system/TimeService.h"

namespace tongdou {

class McpServer {
 public:
  McpServer(TimeService& timeService, ReminderStore& reminderStore,
            HardwareSelfTestService& hardwareSelfTest,
            AppCommandService& appCommandService);

  void begin();
  String handlePayloadText(const String& payloadText);

 private:
  void addTools();
  std::string handlePayload(JsonObjectConst payload);
  std::string replyResult(const std::string& idJson, const std::string& resultJson) const;
  std::string replyError(const std::string& idJson, const std::string& message) const;
  std::string idJson(JsonObjectConst payload) const;
  std::string serializeVariant(JsonVariantConst value) const;
  McpToolResult createReminder(const McpToolArguments& arguments);
  McpToolResult deleteReminder(const McpToolArguments& arguments);
  McpToolResult playScenario(const McpToolArguments& arguments);
  McpToolResult setPersonality(const McpToolArguments& arguments);
  McpToolResult configureVoiceBackend(const McpToolArguments& arguments);
  McpToolResult connectVoiceBackend(const McpToolArguments& arguments);
  McpToolResult voiceDetect(const McpToolArguments& arguments);
  McpToolResult voiceAbort(const McpToolArguments& arguments);
  McpToolResult startVoiceTurn(const McpToolArguments& arguments);
  McpToolResult voiceStatus(const McpToolArguments& arguments);
  McpToolResult voiceCodecSelfTest(const McpToolArguments& arguments);
  McpToolResult playVoiceResponse(const McpToolArguments& arguments);
  McpToolResult failVoiceTurn(const McpToolArguments& arguments);
  std::string commandResultJson(const AppCommandResult& result) const;
  std::string remindersJson() const;
  std::string statusJson() const;

  TimeService& timeService_;
  ReminderStore& reminderStore_;
  HardwareSelfTestService& hardwareSelfTest_;
  AppCommandService& appCommandService_;
  McpToolRegistry registry_;
  bool initialized_ = false;
};

}  // namespace tongdou
