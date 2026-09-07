#pragma once

#include <Arduino.h>

#include "audio/AudioVolumeService.h"
#include "network/NetworkInfoService.h"
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
                    RealtimeVoiceService& realtimeVoiceService,
                    AudioVolumeService& audioVolumeService,
                    NetworkInfoService& networkInfoService);

  AppCommandResult playScenario(const String& eventName);
  AppCommandResult setPersonality(const String& personalityName);
  AppCommandResult configureVoiceBackend(const String& host, int port, const String& path,
                                         const String& token, const String& deviceId,
                                         const String& clientId, bool useTls);
  AppCommandResult connectVoiceBackend();
  AppCommandResult sendVoiceDetect(const String& text);
  AppCommandResult abortVoice();
  AppCommandResult exitVoiceConversation();
  AppCommandResult startVoiceTurn(uint16_t captureMs = 2500);
  AppCommandResult startAutoVoiceTurn();
  AppCommandResult playVoiceResponse(const String& text);
  AppCommandResult failVoiceTurn(const String& reason);
  AppCommandResult setAudioVolume(int volumePercent);
  AppCommandResult adjustAudioVolume(int deltaPercent);
  String voiceStatusJson() const;
  String voiceCodecSelfTestJson();
  const char* personalityName() const;
  uint8_t audioVolume() const;
  NetworkInfoResult showNetworkInfo();

 private:
  bool parseEventName(String name, ScenarioEventType& eventType) const;
  bool parsePersonalityName(String name, PersonalityStyle& style) const;

  ScenarioEngine& scenarioEngine_;
  ScenarioExecutor& scenarioExecutor_;
  ScenarioContext& scenarioContext_;
  RealtimeVoiceService& realtimeVoiceService_;
  AudioVolumeService& audioVolumeService_;
  NetworkInfoService& networkInfoService_;
};

}  // namespace tongdou
