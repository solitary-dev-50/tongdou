#include "scenario/ScenarioExecutor.h"

#include <Arduino.h>

namespace tongdou {
namespace {

constexpr bool kScenarioExecutorDebug = false;
constexpr bool kVoiceClipDebug = false;

const __FlashStringHelper* wheelName(WheelDrive drive) {
  switch (drive) {
    case WheelDrive::Forward:
      return F("forward");
    case WheelDrive::Reverse:
      return F("reverse");
    case WheelDrive::Brake:
      return F("brake");
    case WheelDrive::Stop:
    default:
      return F("stop");
  }
}

}  // namespace

ScenarioExecutor::ScenarioExecutor(FaceDisplay& faceDisplay, LedPixel& led,
                                   MotorDriver& motors, AudioOutput& audio)
    : faceDisplay_(faceDisplay), led_(led), motors_(motors), audio_(audio) {}

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
  motionFrameIndex_ = 0;
  nextMotionFrameMs_ = 0;
  motionRunning_ = false;
  motors_.stop();
}

void ScenarioExecutor::execute(const ScenarioPlan& plan) {
  if (!plan.valid) {
    return;
  }

  activePlan_ = plan;
  if (kScenarioExecutorDebug) {
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
  }

  const unsigned long now = millis();
  startLight(plan.light, now);
  startFace(plan.face, now);
  startMotion(plan.motion, now);
  startVoice(plan.voice);
}

void ScenarioExecutor::stop() {
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
  motionFrameIndex_ = 0;
  nextMotionFrameMs_ = 0;
  motionRunning_ = false;
  motors_.stop();
  audio_.stopStream();
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
  motors_.stop();
  motionSequence_ = motionPack_.sequence(action);
  motionFrameIndex_ = 0;
  nextMotionFrameMs_ = nowMs;
  motionRunning_ = motionSequence_.count > 0;
  updateMotion(nowMs);
}

void ScenarioExecutor::startVoice(VoiceLine line) {
  const VoiceClip clip = voiceLinePack_.clip(line);
  if (clip.clipId == 0) {
    return;
  }

  if (kVoiceClipDebug) {
    Serial.print("voice clip=");
    Serial.print(clip.clipId);
    Serial.print(" file=");
    Serial.print(clip.fileName);
    Serial.print(" text=");
    Serial.println(clip.text);
  }
  audio_.playClip(clip.clipId, clip.fileName, clip.text);
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
  while (motionRunning_ &&
         static_cast<long>(nowMs - nextMotionFrameMs_) >= 0) {
    const MotionFrame& frame = motionSequence_.frames[motionFrameIndex_];
    motors_.drive(frame.left, frame.right, frame.duty, frame.leftDuty,
                  frame.rightDuty);
    ++motionFrameIndex_;
    if (motionFrameIndex_ >= motionSequence_.count) {
      motionRunning_ = false;
      motors_.stop();
      return;
    }
    nextMotionFrameMs_ = nowMs + frame.durationMs;
  }
}

void ScenarioExecutor::update() {
  const unsigned long now = millis();
  updateLight(now);
  updateFace(now);
  updateMotion(now);
}

void ScenarioExecutor::printMotionDebug(MotionAction action, uint32_t relativeT0Ms,
                                        Print& out) const {
  const MotionSequence sequence = motionPack_.sequence(action);
  if (sequence.frames == nullptr || sequence.count == 0) {
    return;
  }

  for (size_t i = 0; i < sequence.count; ++i) {
    const MotionFrame& frame = sequence.frames[i];
    out.print(F("demo motion cue t="));
    out.print(relativeT0Ms);
    out.print(F(" frame="));
    out.print(i);
    out.print(F(" left="));
    out.print(wheelName(frame.left));
    out.print(F(" right="));
    out.print(wheelName(frame.right));
    out.print(F(" duty="));
    out.print(frame.duty);
    out.print(F(" left_pwm="));
    out.print(frame.leftDuty);
    out.print(F(" right_pwm="));
    out.print(frame.rightDuty);
    out.print(F(" duration="));
    out.println(frame.durationMs);
  }
}

uint32_t ScenarioExecutor::lastAudioPlaybackStartedAtMs() const {
  return audio_.lastPlaybackStartedAtMs();
}

}  // namespace tongdou
