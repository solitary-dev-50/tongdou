#pragma once

#include "face/FacePack.h"
#include "hardware/FaceDisplay.h"
#include "hardware/LedPixel.h"
#include "light/LightPack.h"
#include "scenario/ScenarioPlan.h"

namespace tongdou {

class ScenarioExecutor {
 public:
  ScenarioExecutor(FaceDisplay& faceDisplay, LedPixel& led);

  void begin();
  void execute(const ScenarioPlan& plan);
  void update();

 private:
  void playLight(LightAction action);
  void playFace(FaceAction action);

  FaceDisplay& faceDisplay_;
  LedPixel& led_;
  FacePack facePack_;
  LightPack lightPack_;
  ScenarioPlan activePlan_;
};

}  // namespace tongdou
