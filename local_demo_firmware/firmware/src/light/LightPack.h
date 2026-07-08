#pragma once

#include "light/LightTypes.h"
#include "scenario/ScenarioAction.h"

namespace tongdou {

class LightPack {
 public:
  LightSequence sequence(LightAction action) const;
};

}  // namespace tongdou
