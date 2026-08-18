#pragma once

#include <stddef.h>
#include <stdint.h>

#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioRule.h"

namespace tongdou {

constexpr uint16_t kScenarioPackFormatVersion = 1;

struct ScenarioPackInfo {
  uint16_t formatVersion = kScenarioPackFormatVersion;
  const char* id = "";
  const char* name = "";
  const char* description = "";
  PersonalityStyle defaultPersonality = PersonalityStyle::Balanced;
};

struct ScenarioPack {
  ScenarioPackInfo info;
  const ScenarioRule* rules = nullptr;
  size_t ruleCount = 0;
};

}  // namespace tongdou
