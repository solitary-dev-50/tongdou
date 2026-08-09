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

  deviceActivationService_.begin();
  Serial.println("device activation ready");

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

  Serial.println("scenario boot quiet");
  const ScenarioPlan bootPlan =
      scenarioEngine_.select({ScenarioEventType::BootCompleted, 0}, scenarioContext_);
  scenarioExecutor_.execute(bootPlan);

  Serial.println("tongdou app initialized");
}

void App::update() {
  webConfigServer_.update();
  hardware_.update();
  realtimeVoiceService_.update();
  deviceActivationService_.update();
  handleStartupReport();
  handleSerialDiagnostics();
  handleLogoTouchEvents();
  handleReminderEvents();
  handleRealtimeVoiceEvents();
  scenarioExecutor_.update();
}

void App::handleLogoTouchEvents() {
  const LogoTouchEvent touchEvent = hardware_.logoTouch().consumeEvent();
  if (touchEvent == LogoTouchEvent::None) {
    return;
  }

  ScenarioEventType scenarioEvent = ScenarioEventType::LogoSingleTapped;
  const __FlashStringHelper* eventName = F("logo_single_tapped");
  switch (touchEvent) {
    case LogoTouchEvent::SingleTap:
      scenarioEvent = ScenarioEventType::LogoSingleTapped;
      eventName = F("logo_single_tapped");
      break;
    case LogoTouchEvent::DoubleTap:
      scenarioEvent = ScenarioEventType::LogoDoubleTapped;
      eventName = F("logo_double_tapped");
      break;
    case LogoTouchEvent::LongPress:
      scenarioEvent = ScenarioEventType::LogoLongPressed;
      eventName = F("logo_long_pressed");
      break;
    case LogoTouchEvent::None:
    default:
      return;
  }

  Serial.print(F("logo formal event "));
  Serial.println(eventName);
  const ScenarioPlan plan = scenarioEngine_.select({scenarioEvent, 0}, scenarioContext_);
  scenarioExecutor_.execute(plan);
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

  if (hardware_.selfTest().handleCommand(command, Serial)) {
    return;
  }

  Serial.println("unknown command, type help");
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
  Serial.println("  device activation ready");
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
