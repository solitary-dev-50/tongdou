#include "scenario/ScenarioEngine.h"

namespace tongdou {

ScenarioEngine::ScenarioEngine(const ScenarioLibrary& library) : library_(library) {}

ScenarioPlan ScenarioEngine::select(const ScenarioEvent& event,
                                    const ScenarioContext& context) const {
  const ScenarioRule* rules = library_.rules();
  const size_t count = library_.size();

  for (size_t i = 0; i < count; ++i) {
    if (matches(rules[i], event, context)) {
      return rules[i].plan;
    }
  }

  return {};
}

bool ScenarioEngine::matches(const ScenarioRule& rule, const ScenarioEvent& event,
                             const ScenarioContext& context) const {
  if (rule.eventType != event.type) {
    return false;
  }
  if (context.quietMode && !rule.allowInQuietMode) {
    return false;
  }
  if (context.lowBattery && !rule.allowWhenLowBattery) {
    return false;
  }
  if (static_cast<uint8_t>(context.personality) <
      static_cast<uint8_t>(rule.minimumPersonality)) {
    return false;
  }
  return true;
}

}  // namespace tongdou
