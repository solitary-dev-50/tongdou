#pragma once

#include "face/FacePack.h"
#include "hardware/FaceDisplay.h"
#include "scenario/ScenarioPlan.h"

namespace tongdou {

class ScenarioExecutor {
 public:
  explicit ScenarioExecutor(FaceDisplay& faceDisplay);

  void begin();
  void execute(const ScenarioPlan& plan);
  void update();

 private:
  FaceDisplay& faceDisplay_;
  FacePack facePack_;
  ScenarioPlan activePlan_;
};

}  // namespace tongdou
