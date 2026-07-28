#pragma once

#include <stddef.h>
#include <stdint.h>

#include "scenario/ScenarioAction.h"
#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioEvent.h"
#include "scenario/ScenarioPack.h"

namespace tongdou {

constexpr uint8_t kMaxUserScenarioRules = 16;
constexpr size_t kScenarioPackIdLength = 32;
constexpr size_t kScenarioPackNameLength = 48;
constexpr size_t kScenarioPackDescriptionLength = 96;

struct EditableScenarioPackInfo {
  uint16_t formatVersion = kScenarioPackFormatVersion;
  char id[kScenarioPackIdLength] = {};
  char name[kScenarioPackNameLength] = {};
  char description[kScenarioPackDescriptionLength] = {};
  PersonalityStyle defaultPersonality = PersonalityStyle::Balanced;
};

struct UserScenarioRule {
  bool enabled = false;
  ScenarioEventType eventType = ScenarioEventType::ReminderDue;
  bool allowInQuietMode = false;
  bool allowWhenLowBattery = true;
  PersonalityStyle minimumPersonality = PersonalityStyle::Gentle;
  FaceAction face = FaceAction::None;
  LightAction light = LightAction::None;
  MotionAction motion = MotionAction::None;
  VoiceLine voice = VoiceLine::None;
  uint16_t durationMs = 0;
};

struct ScenarioConfig {
  EditableScenarioPackInfo pack;
  bool enableCustomScenarios = false;
  bool enableDramaticEasterEggs = false;
  UserScenarioRule rules[kMaxUserScenarioRules];
};

struct ScenarioConfigPatch {
  ScenarioConfig config;
};

}  // namespace tongdou
