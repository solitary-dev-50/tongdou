#pragma once

#include <stdint.h>

#include "scenario/ScenarioAction.h"

namespace tongdou {

struct ScenarioPlan {
  bool valid = false;
  FaceAction face = FaceAction::None;
  LightAction light = LightAction::None;
  MotionAction motion = MotionAction::None;
  VoiceLine voice = VoiceLine::None;
  uint16_t durationMs = 0;
};

}  // namespace tongdou
