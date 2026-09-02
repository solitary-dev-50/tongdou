#include "scenario/ScenarioExecutor.h"

#include <Arduino.h>

namespace tongdou {

ScenarioExecutor::ScenarioExecutor(FaceDisplay& faceDisplay, LedPixel& led,
                                   MotorDriver& motors, AudioOutput& audio,
                                   ImuSensor& imu)
    : faceDisplay_(faceDisplay),
      led_(led),
      motors_(motors),
      audio_(audio),
      gyroMotion_(motors, imu) {}

void ScenarioExecutor::begin() {
  activePlan_ = {};
  faceFrames_ = nullptr;
  faceFrameCount_ = 0;
  faceFrameIndex_ = 0;
  nextFaceFrameMs_ = 0;
  faceRunning_ = false;
  lightSequence_ = {};
  lightFrameIndex_ = 0;
  nextLightFrameMs_ = 0;
  lightRunning_ = false;
  motionSequence_ = {};
  gyroMotion_.begin();
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

  const unsigned long now = millis();
  startLight(plan.light, now);
  startFace(plan.face, now);
  startMotion(plan.motion, now);
  startVoice(plan.voice);
}

void ScenarioExecutor::finishActivePlan() {
  activePlan_ = {};
  faceFrames_ = nullptr;
  faceFrameCount_ = 0;
  faceFrameIndex_ = 0;
  nextFaceFrameMs_ = 0;
  faceRunning_ = false;
  lightSequence_ = {};
  lightFrameIndex_ = 0;
  nextLightFrameMs_ = 0;
  lightRunning_ = false;
  motionSequence_ = {};
  gyroMotion_.stop();
  faceDisplay_.show(FaceExpression::Sleep);
  led_.off();
  Serial.println(F("scenario active plan finished state=idle"));
}

void ScenarioExecutor::startLight(LightAction action, unsigned long nowMs) {
  lightSequence_ = lightPack_.sequence(action);
  lightFrameIndex_ = 0;
  nextLightFrameMs_ = nowMs;
  lightRunning_ = lightSequence_.count > 0;
  updateLight(nowMs);
}

void ScenarioExecutor::startFace(FaceAction action, unsigned long nowMs) {
  faceFrameCount_ = 0;
  faceFrames_ = facePack_.frames(action, faceFrameCount_);
  faceFrameIndex_ = 0;
  nextFaceFrameMs_ = nowMs;
  faceRunning_ = faceFrames_ != nullptr && faceFrameCount_ > 0;
  updateFace(nowMs);
}

void ScenarioExecutor::startMotion(MotionAction action, unsigned long nowMs) {
  motionSequence_ = motionPack_.sequence(action);
  gyroMotion_.start(motionSequence_);
}

void ScenarioExecutor::startVoice(VoiceLine line) {
  const VoiceClip clip = voiceLinePack_.clip(line);
  if (clip.clipId == 0) {
    return;
  }

  Serial.print("voice clip=");
  Serial.print(clip.clipId);
  Serial.print(" text=");
  Serial.println(clip.text);
  audio_.playClip(clip.clipId, clip.text);
}

void ScenarioExecutor::updateLight(unsigned long nowMs) {
  while (lightRunning_ &&
         static_cast<long>(nowMs - nextLightFrameMs_) >= 0) {
    const LightFrame& frame = lightSequence_.frames[lightFrameIndex_];
    led_.show(frame.color);
    ++lightFrameIndex_;
    if (lightFrameIndex_ >= lightSequence_.count) {
      lightRunning_ = false;
      return;
    }
    nextLightFrameMs_ = nowMs + frame.durationMs;
  }
}

void ScenarioExecutor::updateFace(unsigned long nowMs) {
  while (faceRunning_ &&
         static_cast<long>(nowMs - nextFaceFrameMs_) >= 0) {
    const FaceFrame& frame = faceFrames_[faceFrameIndex_];
    faceDisplay_.show(frame.expression);
    ++faceFrameIndex_;
    if (faceFrameIndex_ >= faceFrameCount_) {
      faceRunning_ = false;
      return;
    }
    nextFaceFrameMs_ = nowMs + frame.durationMs;
  }
}

void ScenarioExecutor::updateMotion(unsigned long nowMs) {
  gyroMotion_.update();
}

void ScenarioExecutor::update() {
  const unsigned long now = millis();
  updateLight(now);
  updateFace(now);
  updateMotion(now);
}

}  // namespace tongdou
