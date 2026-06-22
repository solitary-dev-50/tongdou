#pragma once

#include <Arduino.h>

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
                    ScenarioContext& scenarioContext);

  AppCommandResult playScenario(const String& eventName);
  AppCommandResult setPersonality(const String& personalityName);
  const char* personalityName() const;

 private:
  bool parseEventName(String name, ScenarioEventType& eventType) const;
  bool parsePersonalityName(String name, PersonalityStyle& style) const;

  ScenarioEngine& scenarioEngine_;
  ScenarioExecutor& scenarioExecutor_;
  ScenarioContext& scenarioContext_;
};

}  // namespace tongdou
