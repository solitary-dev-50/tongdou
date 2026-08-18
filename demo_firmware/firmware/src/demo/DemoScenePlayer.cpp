#include "demo/DemoScenePlayer.h"

namespace tongdou {
namespace {

constexpr bool kDemoTimelineDebug = false;

}  // namespace

DemoScenePlayer::DemoScenePlayer(ScenarioExecutor& scenarioExecutor,
                                 RealtimeVoiceService& realtimeVoiceService)
    : scenarioExecutor_(scenarioExecutor),
      realtimeVoiceService_(realtimeVoiceService) {}

void DemoScenePlayer::begin() {
  preferences_.begin("td_demo", false);
  stop();
}

void DemoScenePlayer::update() {
  if (!playing_) {
    return;
  }

  const unsigned long now = millis();
  if (preludeActive_) {
    const unsigned long preludeElapsed = now - preludeStartedMs_;
    while (nextPreludeStepIndex_ < activeScene_.preludeStepCount &&
           preludeElapsed >=
               activeScene_.preludeSteps[nextPreludeStepIndex_].atMs) {
      executeStep(activeScene_.preludeSteps[nextPreludeStepIndex_],
                  preludeElapsed);
      ++nextPreludeStepIndex_;
    }

    const DemoSceneStep& lastPrelude =
        activeScene_.preludeSteps[activeScene_.preludeStepCount - 1];
    const unsigned long preludeEndMs =
        static_cast<unsigned long>(lastPrelude.atMs) +
        lastPrelude.plan.durationMs;
    if (preludeElapsed < preludeEndMs) {
      return;
    }

    preludeActive_ = false;
    startedMs_ = now;
    nextStepIndex_ = 0;
    Serial.print(F("demo prelude complete scene="));
    Serial.println(demoSceneSlug(currentId_, currentVariant_));
  }

  if (syncToVoiceStart_ && voiceStepIssued_ && !audioTimebaseActive_) {
    const uint32_t t0 = scenarioExecutor_.lastAudioPlaybackStartedAtMs();
    if (t0 == 0 || static_cast<int32_t>(t0 - audioStartWatchMs_) < 0) {
      return;
    }

    audioStartedMs_ = t0;
    audioTimebaseActive_ = true;
    Serial.print(F("demo sync t0 scene="));
    Serial.print(demoSceneSlug(currentId_));
    Serial.print(F(" ms="));
    Serial.println(audioStartedMs_);
  }

  const unsigned long elapsed =
      syncToVoiceStart_ && audioTimebaseActive_
          ? now - audioStartedMs_
          : now - startedMs_;
  while (playing_ && nextStepIndex_ < activeScene_.stepCount &&
         elapsed >= activeScene_.steps[nextStepIndex_].atMs) {
    executeStep(activeScene_.steps[nextStepIndex_], elapsed);
    ++nextStepIndex_;
    if (syncToVoiceStart_ && voiceStepIssued_ && !audioTimebaseActive_) {
      return;
    }
  }

  if (nextStepIndex_ >= activeScene_.stepCount) {
    if (syncToVoiceStart_ && !audioTimebaseActive_) {
      return;
    }

    const DemoSceneStep& lastStep = activeScene_.steps[activeScene_.stepCount - 1];
    const unsigned long sceneEndMs =
        static_cast<unsigned long>(lastStep.atMs) + lastStep.plan.durationMs;
    if (elapsed < sceneEndMs) {
      return;
    }

    playing_ = false;
    if (autoFirstSummonActive_ && currentId_ == DemoSceneId::FirstSummonNightmare) {
      markFirstSummonPlayed();
    }
    autoFirstSummonActive_ = false;
  }
}

bool DemoScenePlayer::play(DemoSceneId id) {
  return play(id, 0);
}

bool DemoScenePlayer::play(DemoSceneId id, uint8_t variant) {
  if (id == DemoSceneId::IdleStop) {
    stop();
    return true;
  }

  const DemoScene scene = library_.scene(id, variant);
  if (scene.steps == nullptr || scene.stepCount == 0) {
    return false;
  }

  realtimeVoiceService_.abortBackendSpeech();
  scenarioExecutor_.stop();

  activeScene_ = scene;
  currentId_ = id;
  currentVariant_ = scene.variant;
  startedMs_ = millis();
  nextStepIndex_ = 0;
  preludeStartedMs_ = startedMs_;
  nextPreludeStepIndex_ = 0;
  preludeActive_ =
      scene.preludeSteps != nullptr && scene.preludeStepCount > 0;
  playing_ = true;
  syncToVoiceStart_ = scene.syncToVoiceStart;
  voiceStepIssued_ = false;
  audioTimebaseActive_ = false;
  audioStartWatchMs_ = 0;
  audioStartedMs_ = 0;

  if (id == DemoSceneId::FirstSummonNightmare && activeScene_.steps[0].atMs == 0) {
    executeStep(activeScene_.steps[0], 0);
    nextStepIndex_ = 1;
  }

  Serial.print("demo scene ");
  Serial.print(static_cast<int>(id));
  Serial.print(" ");
  Serial.print(demoSceneSlug(id, currentVariant_));
  if (currentVariant_ > 0) {
    Serial.print(" variant=");
    Serial.print(currentVariant_);
  }
  Serial.println();

  if (scene.startsRealtimeVoice) {
    realtimeVoiceService_.startTurn();
  }

  update();
  return true;
}

bool DemoScenePlayer::playFirstSummonIfNeeded() {
  if (preferences_.getBool("summon_done", false)) {
    return false;
  }

  if (!play(DemoSceneId::FirstSummonNightmare)) {
    return false;
  }

  autoFirstSummonActive_ = true;
  Serial.println("first summon auto playback started");
  return true;
}

void DemoScenePlayer::stop() {
  realtimeVoiceService_.abortBackendSpeech();
  scenarioExecutor_.stop();
  currentId_ = DemoSceneId::IdleStop;
  currentVariant_ = 0;
  activeScene_ = {};
  startedMs_ = 0;
  nextStepIndex_ = 0;
  preludeStartedMs_ = 0;
  nextPreludeStepIndex_ = 0;
  preludeActive_ = false;
  playing_ = false;
  autoFirstSummonActive_ = false;
  syncToVoiceStart_ = false;
  voiceStepIssued_ = false;
  audioTimebaseActive_ = false;
  audioStartWatchMs_ = 0;
  audioStartedMs_ = 0;
  Serial.println("demo stop");
  executeIdle();
}

bool DemoScenePlayer::handleSerialCommand(const String& command, Print& out) {
  if (command == "0" || command == "stop" || command == "idle") {
    stop();
    out.println("demo scene 0 idle_stop");
    return true;
  }

  if (command == "6" || command == "6-1" || command == "accountant" ||
      command == "accountant 1") {
    play(DemoSceneId::ConfusedAccountantGlobal, 1);
    out.println("demo scene 6-1 confused_accountant_clear");
    return true;
  }

  if (command == "6-2" || command == "accountant 2") {
    play(DemoSceneId::ConfusedAccountantGlobal, 2);
    out.println("demo scene 6-2 confused_accountant_extort");
    return true;
  }

  if (command == "6-3" || command == "accountant 3") {
    play(DemoSceneId::ConfusedAccountantGlobal, 3);
    out.println("demo scene 6-3 confused_accountant_bribe");
    return true;
  }

  if (command == "6-4" || command == "accountant 4") {
    play(DemoSceneId::ConfusedAccountantGlobal, 4);
    out.println("demo scene 6-4 confused_accountant_forgetful");
    return true;
  }

  if (command == "6-5" || command == "accountant 5") {
    play(DemoSceneId::ConfusedAccountantGlobal, 5);
    out.println("demo scene 6-5 confused_accountant_pi");
    return true;
  }

  if (command == "summon" || command == "nightmare") {
    play(DemoSceneId::FirstSummonNightmare);
    out.println("demo scene 8 first_summon_nightmare");
    return true;
  }

  if (command == "reset_first_summon") {
    preferences_.putBool("summon_done", false);
    out.println("first_summon_played reset");
    return true;
  }

  return false;
}

String DemoScenePlayer::statusJson() const {
  String body;
  body.reserve(180);
  body += "{\"ok\":true,\"id\":";
  body += static_cast<int>(currentId_);
  body += ",\"variant\":";
  body += currentVariant_;
  body += ",\"scene\":\"";
  body += demoSceneSlug(currentId_, currentVariant_);
  body += "\",\"title\":\"";
  body += demoSceneTitle(currentId_, currentVariant_);
  body += "\",\"playing\":";
  body += playing_ ? "true" : "false";
  body += "}";
  return body;
}

void DemoScenePlayer::executeIdle() {
  scenarioExecutor_.execute({true, FaceAction::Awake, LightAction::WeakBreath,
                             MotionAction::Stop, VoiceLine::None, 0});
}

void DemoScenePlayer::markFirstSummonPlayed() {
  preferences_.putBool("summon_done", true);
  Serial.println("first_summon_played saved");
}

void DemoScenePlayer::executeStep(const DemoSceneStep& step,
                                  unsigned long relativeMs) {
  const unsigned long cueTime =
      syncToVoiceStart_ && audioTimebaseActive_ ? relativeMs : step.atMs;
  if (kDemoTimelineDebug) {
    Serial.print(F("demo cue scene="));
    Serial.print(demoSceneSlug(currentId_, currentVariant_));
    Serial.print(F(" t="));
    Serial.print(cueTime);
    Serial.print(F(" face="));
    Serial.print(static_cast<int>(step.plan.face));
    Serial.print(F(" light="));
    Serial.print(static_cast<int>(step.plan.light));
    Serial.print(F(" motion="));
    Serial.print(static_cast<int>(step.plan.motion));
    Serial.print(F(" voice="));
    Serial.println(static_cast<int>(step.plan.voice));

    if ((currentId_ == DemoSceneId::FirstSummonNightmare ||
         currentId_ == DemoSceneId::ConfusedAccountantGlobal) &&
        step.plan.motion != MotionAction::None) {
      scenarioExecutor_.printMotionDebug(step.plan.motion, cueTime, Serial);
    }
  }

  scenarioExecutor_.execute(step.plan);
  if (syncToVoiceStart_ && step.plan.voice != VoiceLine::None) {
    voiceStepIssued_ = true;
    audioStartWatchMs_ = millis();
  }
}

}  // namespace tongdou
