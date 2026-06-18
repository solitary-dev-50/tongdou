#pragma once

#include <stdint.h>

#include "scenario/ScenarioAction.h"
#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioEvent.h"

namespace tongdou {

constexpr uint8_t kMaxUserScenarioRules = 16;

struct UserScenarioRule {
  bool enabled = false;
  ScenarioEventType eventType = ScenarioEventType::ReminderDue;
  PersonalityStyle personality = PersonalityStyle::Balanced;
  FaceAction face = FaceAction::None;
  LightAction light = LightAction::None;
  MotionAction motion = MotionAction::None;
  VoiceLine voice = VoiceLine::None;
  uint16_t durationMs = 0;
};

struct ScenarioConfig {
  PersonalityStyle defaultPersonality = PersonalityStyle::Balanced;
  bool enableCustomScenarios = false;
  bool enableDramaticEasterEggs = false;
  UserScenarioRule rules[kMaxUserScenarioRules];
};

}  // namespace tongdou
