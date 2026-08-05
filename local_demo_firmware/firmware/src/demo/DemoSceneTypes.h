#pragma once

#include <stddef.h>
#include <stdint.h>

#include "demo/DemoSceneId.h"
#include "scenario/ScenarioPlan.h"

namespace tongdou {

struct DemoSceneStep {
  uint16_t atMs = 0;
  ScenarioPlan plan;
};

struct DemoScene {
  DemoSceneId id = DemoSceneId::IdleStop;
  uint8_t variant = 0;
  const DemoSceneStep* steps = nullptr;
  size_t stepCount = 0;
  bool startsRealtimeVoice = false;
  bool syncToVoiceStart = false;
  const DemoSceneStep* preludeSteps = nullptr;
  size_t preludeStepCount = 0;
};

}  // namespace tongdou
