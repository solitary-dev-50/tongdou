#pragma once

#include "motion/MotionTypes.h"
#include "scenario/ScenarioAction.h"

namespace tongdou {

class MotionPack {
 public:
  MotionSequence sequence(MotionAction action) const;
};

}  // namespace tongdou
