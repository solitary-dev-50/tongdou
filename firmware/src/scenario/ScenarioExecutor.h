#pragma once

#include "face/FacePack.h"
#include "hardware/AudioOutput.h"
#include "hardware/FaceDisplay.h"
#include "hardware/ImuSensor.h"
#include "hardware/LedPixel.h"
#include "hardware/MotorDriver.h"
#include "motion/GyroMotionController.h"
#include "light/LightPack.h"
#include "motion/MotionPack.h"
#include "scenario/ScenarioPlan.h"
#include "voice/VoiceLinePack.h"

namespace tongdou {

class ScenarioExecutor {
 public:
  ScenarioExecutor(FaceDisplay& faceDisplay, LedPixel& led, MotorDriver& motors,
                   AudioOutput& audio, ImuSensor& imu);

  void begin();
  void execute(const ScenarioPlan& plan);
  void finishActivePlan();
  void update();

 private:
  void startLight(LightAction action, unsigned long nowMs);
  void startFace(FaceAction action, unsigned long nowMs);
  void startMotion(MotionAction action, unsigned long nowMs);
  void startVoice(VoiceLine line);
  void updateLight(unsigned long nowMs);
  void updateFace(unsigned long nowMs);
  void updateMotion(unsigned long nowMs);

  FaceDisplay& faceDisplay_;
  LedPixel& led_;
  MotorDriver& motors_;
  AudioOutput& audio_;
  GyroMotionController gyroMotion_;
  FacePack facePack_;
  LightPack lightPack_;
  MotionPack motionPack_;
  VoiceLinePack voiceLinePack_;
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
  MotionSequence motionSequence_;
};

}  // namespace tongdou
