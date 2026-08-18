#pragma once

#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioEvent.h"
#include "scenario/ScenarioPlan.h"

namespace tongdou {

struct ScenarioRule {
  ScenarioEventType eventType = ScenarioEventType::BootCompleted;
  bool allowInQuietMode = false;
  bool allowWhenLowBattery = true;
  PersonalityStyle minimumPersonality = PersonalityStyle::Gentle;
  ScenarioPlan plan;
};

}  // namespace tongdou
