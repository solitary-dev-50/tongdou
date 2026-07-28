#include "app/App.h"

#include <WiFi.h>

#include "tongdou/BuildInfo.h"

namespace tongdou {
namespace {

constexpr unsigned long kFirstStartupReportMs = 2500;
constexpr unsigned long kSecondStartupReportMs = 8000;

}  // namespace

void App::begin() {
  Serial.begin(115200);
  bootMs_ = millis();
  hardware_.begin();

  delay(200);
  Serial.println();
  Serial.print(kProjectName);
  Serial.print(" firmware stage: ");
  Serial.println(kFirmwareStage);

  scenarioExecutor_.begin();
  Serial.println("scenario executor ready");

  realtimeVoiceService_.begin();
  Serial.println("realtime voice ready");

  demoScenePlayer_.begin();
  Serial.println("demo scene player ready");

  wifiConfigStore_.begin();
  Serial.println("wifi config ready");

  timeService_.begin();
  Serial.println("time service started");

  reminderStore_.begin();
  Serial.println("reminder store ready");

  reminderManager_.begin();
  Serial.println("reminder manager ready");

  mcpServer_.begin();
  Serial.println("mcp server ready");

  scenarioConfigStore_.begin();
  Serial.println("scenario config ready");

  webConfigServer_.begin();

  if (!demoScenePlayer_.playFirstSummonIfNeeded()) {
    Serial.println("scenario boot quiet");
    const ScenarioPlan bootPlan =
        scenarioEngine_.select({ScenarioEventType::BootCompleted, 0}, scenarioContext_);
    scenarioExecutor_.execute(bootPlan);
  }

  Serial.println("tongdou app initialized");
}

void App::update() {
  webConfigServer_.update();
  hardware_.update();
  realtimeVoiceService_.update();
  demoScenePlayer_.update();
  handleStartupReport();
  handleSerialDiagnostics();
  handleLogoTouchEvents();
  handleReminderEvents();
  handleRealtimeVoiceEvents();
  scenarioExecutor_.update();
  logoWiggle_.update();
}

void App::handleReminderEvents() {
  ScenarioEvent event;
  if (!reminderManager_.update(event)) {
    return;
  }

  const ScenarioPlan plan = scenarioEngine_.select(event, scenarioContext_);
  scenarioExecutor_.execute(plan);
}

void App::handleRealtimeVoiceEvents() {
  if (!realtimeVoiceService_.consumeFailureEvent()) {
    return;
  }

  const ScenarioPlan plan = scenarioEngine_.select(
      {ScenarioEventType::VoiceRecognitionFailed, 0}, scenarioContext_);
  scenarioExecutor_.execute(plan);
}

void App::handleLogoTouchEvents() {
  const LogoTouchEvent event = hardware_.logoTouch().consumeEvent();
  if (event == LogoTouchEvent::None) {
    return;
  }

  logoWiggle_.stop();

  ScenarioPlan plan;
  plan.valid = true;
  plan.face = FaceAction::Blink;
  plan.light = LightAction::None;
  plan.voice = VoiceLine::None;
  plan.durationMs = 650;

  if (event == LogoTouchEvent::LongPress) {
    logoSleepMode_ = !logoSleepMode_;
    plan.face = logoSleepMode_ ? FaceAction::Sleep : FaceAction::WakeUp;
    plan.light = logoSleepMode_ ? LightAction::Off : LightAction::WeakBreath;
    plan.motion = MotionAction::Stop;
    plan.durationMs = logoSleepMode_ ? 800 : 1200;
    Serial.println(logoSleepMode_ ? "logo action=long_press_sleep"
                                  : "logo action=long_press_wake");
  } else if (event == LogoTouchEvent::DoubleTap) {
    plan.face = FaceAction::Proud;
    plan.motion = MotionAction::None;
    plan.durationMs = 1600;
    Serial.println("logo action=double_tap_gyro_wiggle");
  } else {
    plan.motion = MotionAction::None;
    Serial.println("logo action=single_tap_blink");
  }

  scenarioExecutor_.execute(plan);
  if (event == LogoTouchEvent::DoubleTap) {
    logoWiggle_.start();
  }
}

void App::handleSerialDiagnostics() {
  if (!Serial.available()) {
    return;
  }

  const String command = Serial.readStringUntil('\n');
  if (command.length() == 0) {
    return;
  }

  String trimmed = command;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return;
  }

  if (demoScenePlayer_.handleSerialCommand(trimmed, Serial)) {
    return;
  }

  Serial.println("unknown demo command");
}

void App::handleStartupReport() {
  const unsigned long elapsed = millis() - bootMs_;
  if (startupReportCount_ == 0 && elapsed >= kFirstStartupReportMs) {
    printStartupReport("startup report");
    startupReportCount_ = 1;
    return;
  }

  if (startupReportCount_ == 1 && elapsed >= kSecondStartupReportMs) {
    printStartupReport("startup report repeat");
    startupReportCount_ = 2;
  }
}

void App::printStartupReport(const char* title) {
  Serial.println();
  Serial.println(title);
  Serial.print(kProjectName);
  Serial.print(" firmware stage: ");
  Serial.println(kFirmwareStage);
  hardware_.printStartupReport(Serial);
  Serial.println("application begin");
  Serial.println("  scenario executor ready");
  Serial.println("  realtime voice ready");
  Serial.println("  wifi config ready");
  Serial.println("  time service started");
  Serial.println("  reminder store ready");
  Serial.println("  reminder manager ready");
  Serial.println("  mcp server ready");
  Serial.println("  scenario config ready");
  Serial.println("  web config server started");
  Serial.println("  scenario boot quiet");
  Serial.print("  wifi ");
  if (WiFi.isConnected()) {
    Serial.print("connected ip=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("not connected");
  }
  Serial.println("application ready");
}
}  // namespace tongdou
