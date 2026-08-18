#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "demo/DemoSceneLibrary.h"
#include "realtime/RealtimeVoiceService.h"
#include "scenario/ScenarioExecutor.h"

namespace tongdou {

class DemoScenePlayer {
 public:
  DemoScenePlayer(ScenarioExecutor& scenarioExecutor,
                  RealtimeVoiceService& realtimeVoiceService);

  void begin();
  void update();
  bool play(DemoSceneId id);
  bool play(DemoSceneId id, uint8_t variant);
  bool playFirstSummonIfNeeded();
  void stop();
  bool handleSerialCommand(const String& command, Print& out);
  String statusJson() const;

 private:
  void executeIdle();
  void markFirstSummonPlayed();
  void executeStep(const DemoSceneStep& step, unsigned long relativeMs);

  ScenarioExecutor& scenarioExecutor_;
  RealtimeVoiceService& realtimeVoiceService_;
  Preferences preferences_;
  DemoSceneLibrary library_;
  DemoScene activeScene_;
  DemoSceneId currentId_ = DemoSceneId::IdleStop;
  uint8_t currentVariant_ = 0;
  unsigned long startedMs_ = 0;
  size_t nextStepIndex_ = 0;
  unsigned long preludeStartedMs_ = 0;
  size_t nextPreludeStepIndex_ = 0;
  bool preludeActive_ = false;
  bool playing_ = false;
  bool autoFirstSummonActive_ = false;
  bool syncToVoiceStart_ = false;
  bool voiceStepIssued_ = false;
  bool audioTimebaseActive_ = false;
  uint32_t audioStartWatchMs_ = 0;
  uint32_t audioStartedMs_ = 0;
};

}  // namespace tongdou
