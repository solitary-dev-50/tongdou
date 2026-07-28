#pragma once

#include <Arduino.h>

#include "realtime/RealtimeVoiceService.h"
#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioEngine.h"
#include "scenario/ScenarioExecutor.h"

namespace tongdou {

struct AppCommandResult {
  bool ok = false;
  String code;
  String message;
};

class AppCommandService {
 public:
  AppCommandService(ScenarioEngine& scenarioEngine,
                    ScenarioExecutor& scenarioExecutor,
                    ScenarioContext& scenarioContext,
                    RealtimeVoiceService& realtimeVoiceService);

  AppCommandResult playScenario(const String& eventName);
  AppCommandResult setPersonality(const String& personalityName);
  AppCommandResult configureVoiceBackend(const String& host, int port, const String& path,
                                         const String& token, const String& deviceId,
                                         const String& clientId, bool useTls);
  AppCommandResult connectVoiceBackend();
  AppCommandResult sendVoiceDetect(const String& text);
  AppCommandResult abortVoice();
  AppCommandResult startVoiceTurn(uint16_t captureMs = 2500);
  AppCommandResult playVoiceResponse(const String& text);
  AppCommandResult failVoiceTurn(const String& reason);
  String voiceStatusJson() const;
  String voiceCodecSelfTestJson();
  const char* personalityName() const;

 private:
  bool parseEventName(String name, ScenarioEventType& eventType) const;
  bool parsePersonalityName(String name, PersonalityStyle& style) const;

  ScenarioEngine& scenarioEngine_;
  ScenarioExecutor& scenarioExecutor_;
  ScenarioContext& scenarioContext_;
  RealtimeVoiceService& realtimeVoiceService_;
};

}  // namespace tongdou
