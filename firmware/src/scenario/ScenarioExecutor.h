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
  void startLight(LightAction action, unsigned long nowMs);
  void startFace(FaceAction action, unsigned long nowMs);
  void updateLight(unsigned long nowMs);
  void updateFace(unsigned long nowMs);

  FaceDisplay& faceDisplay_;
  LedPixel& led_;
  FacePack facePack_;
  LightPack lightPack_;
  ScenarioPlan activePlan_;
  const FaceFrame* faceFrames_ = nullptr;
  size_t faceFrameCount_ = 0;
  size_t faceFrameIndex_ = 0;
  unsigned long nextFaceFrameMs_ = 0;
  bool faceRunning_ = false;
  LightSequence lightSequence_;
  size_t lightFrameIndex_ = 0;
  unsigned long nextLightFrameMs_ = 0;
  bool lightRunning_ = false;
};

}  // namespace tongdou
