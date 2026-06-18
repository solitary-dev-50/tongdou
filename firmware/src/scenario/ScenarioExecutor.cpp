#include "scenario/ScenarioExecutor.h"

#include <Arduino.h>

namespace tongdou {

ScenarioExecutor::ScenarioExecutor(FaceDisplay& faceDisplay) : faceDisplay_(faceDisplay) {}

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

  size_t frameCount = 0;
  const FaceFrame* frames = facePack_.frames(plan.face, frameCount);
  for (size_t i = 0; i < frameCount; ++i) {
    faceDisplay_.show(frames[i].expression);
    if (frames[i].durationMs > 0) {
      delay(frames[i].durationMs);
    }
  }
}

void ScenarioExecutor::update() {}

}  // namespace tongdou
