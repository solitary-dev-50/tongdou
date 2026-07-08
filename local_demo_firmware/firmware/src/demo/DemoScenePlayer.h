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
  bool playFirstSummonIfNeeded();
  void stop();
  bool handleSerialCommand(const String& command, Print& out);
  String statusJson() const;

 private:
  void executeIdle();
  void markFirstSummonPlayed();

  ScenarioExecutor& scenarioExecutor_;
  RealtimeVoiceService& realtimeVoiceService_;
  Preferences preferences_;
  DemoSceneLibrary library_;
  DemoScene activeScene_;
  DemoSceneId currentId_ = DemoSceneId::IdleStop;
  unsigned long startedMs_ = 0;
  size_t nextStepIndex_ = 0;
  bool playing_ = false;
  bool autoFirstSummonActive_ = false;
};

}  // namespace tongdou
