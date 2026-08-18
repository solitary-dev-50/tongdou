#pragma once

#include <stddef.h>

#include "face/FaceExpression.h"
#include "scenario/ScenarioAction.h"

namespace tongdou {

class FacePack {
 public:
  const FaceFrame* frames(FaceAction action, size_t& count) const;
};

}  // namespace tongdou
