#include "scenario/ScenarioExecutor.h"

#include <Arduino.h>

namespace tongdou {

void ScenarioExecutor::begin() {
  activePlan_ = {};
}

void ScenarioExecutor::execute(const ScenarioPlan& plan) {
  if (!plan.valid) {
    return;
  }

  activePlan_ = plan;
  Serial.print("scenario plan face=");
  Serial.print(static_cast<int>(plan.face));
  Serial.print(" light=");
  Serial.print(static_cast<int>(plan.light));
  Serial.print(" motion=");
  Serial.print(static_cast<int>(plan.motion));
  Serial.print(" voice=");
  Serial.print(static_cast<int>(plan.voice));
  Serial.print(" duration=");
  Serial.println(plan.durationMs);
}

void ScenarioExecutor::update() {}

}  // namespace tongdou
