#pragma once

#include <stddef.h>

#include "scenario/ScenarioAction.h"
#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioEvent.h"

namespace tongdou {

class ScenarioConfigOptions {
 public:
  const PersonalityStyle* personalities(size_t& count) const;
  const ScenarioEventType* events(size_t& count) const;
  const FaceAction* faces(size_t& count) const;
  const LightAction* lights(size_t& count) const;
  const MotionAction* motions(size_t& count) const;
  const VoiceLine* voices(size_t& count) const;
};

}  // namespace tongdou
