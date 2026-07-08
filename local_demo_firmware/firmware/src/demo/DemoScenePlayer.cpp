#include "demo/DemoScenePlayer.h"

namespace tongdou {

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

  const unsigned long elapsed = millis() - startedMs_;
  while (playing_ && nextStepIndex_ < activeScene_.stepCount &&
         elapsed >= activeScene_.steps[nextStepIndex_].atMs) {
    scenarioExecutor_.execute(activeScene_.steps[nextStepIndex_].plan);
    ++nextStepIndex_;
  }

  if (nextStepIndex_ >= activeScene_.stepCount) {
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
  if (id == DemoSceneId::IdleStop) {
    stop();
    return true;
  }

  const DemoScene scene = library_.scene(id);
  if (scene.steps == nullptr || scene.stepCount == 0) {
    return false;
  }

  activeScene_ = scene;
  currentId_ = id;
  startedMs_ = millis();
  nextStepIndex_ = 0;
  playing_ = true;

  Serial.print("demo scene ");
  Serial.print(static_cast<int>(id));
  Serial.print(" ");
  Serial.println(demoSceneSlug(id));

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
  activeScene_ = {};
  startedMs_ = 0;
  nextStepIndex_ = 0;
  playing_ = false;
  autoFirstSummonActive_ = false;
  Serial.println("demo stop");
  executeIdle();
}

bool DemoScenePlayer::handleSerialCommand(const String& command, Print& out) {
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
  body += ",\"scene\":\"";
  body += demoSceneSlug(currentId_);
  body += "\",\"title\":\"";
  body += demoSceneTitle(currentId_);
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

}  // namespace tongdou
