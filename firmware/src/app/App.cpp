#include "app/App.h"

#include <WiFi.h>

#include "tongdou/BuildInfo.h"

namespace tongdou {
namespace {

constexpr unsigned long kFirstStartupReportMs = 2500;
constexpr unsigned long kSecondStartupReportMs = 8000;
constexpr unsigned long kConversationPlaybackTailMs = 120;

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

  localAudioPipeline_.begin();
  Serial.println("local audio pipeline ready");

  realtimeVoiceService_.begin();
  Serial.println("realtime voice ready");

  deviceActivationService_.begin();
  Serial.println("device activation ready");

  wifiConfigStore_.begin();
  Serial.println("wifi config ready");

  deviceLocaleStore_.begin();
  Serial.print("device locale=");
  Serial.println(deviceLocaleStore_.locale());

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
  localAudioPipeline_.update();
  realtimeVoiceService_.update();
  deviceActivationService_.update();
  handleAudioCueEvents();
  handleStartupReport();
  handleSerialDiagnostics();
  handleLogoTouchEvents();
  handleReminderEvents();
  handleRealtimeVoiceEvents();
  handleConversationWindow();
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

  if (touchEvent == LogoTouchEvent::SingleTap) {
    startLogoVoiceTurn();
  } else if (touchEvent == LogoTouchEvent::LongPress) {
    requestConversationExitFromInteraction(F("logo_long_press"));
  }
}

void App::startLogoVoiceTurn() {
  closeConversationWindow(false);
  startVoiceTurnFromInteraction(F("logo"));
}

void App::startConversationVoiceTurn() {
  Serial.print(F("voice timeline app_start t="));
  Serial.print(millis());
  Serial.println(F(" source=conversation_follow_up pre_roll_samples=0"));

  const bool started = realtimeVoiceService_.startConversationTurn();
  const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
  Serial.print(F("voice turn start source=conversation_follow_up ok="));
  Serial.print(started ? "1" : "0");
  Serial.print(F(" code="));
  Serial.print(started ? String("ok") : snapshot.lastErrorCode);
  Serial.print(F(" message="));
  Serial.println(started ? String("voice turn started")
                         : snapshot.lastErrorMessage);
  if (!started) {
    voiceConversationActive_ = false;
  }
}

void App::startVoiceTurnFromInteraction(const __FlashStringHelper* source) {
  Serial.print(F("voice timeline app_start t="));
  Serial.print(millis());
  Serial.print(F(" source="));
  Serial.print(source);
  Serial.println(F(" pre_roll_samples=0"));
  const bool started = realtimeVoiceService_.startAutoTurn();
  const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
  const AppCommandResult result =
      started ? AppCommandResult{true, "ok", "voice turn started"}
              : AppCommandResult{false, snapshot.lastErrorCode,
                                 snapshot.lastErrorMessage};
  Serial.print(F("voice turn start source="));
  Serial.print(source);
  Serial.print(F(" ok="));
  Serial.print(result.ok ? "1" : "0");
  Serial.print(F(" code="));
  Serial.print(result.code);
  Serial.print(F(" message="));
  Serial.println(result.message);
  voiceConversationActive_ = result.ok;
  conversationFollowUpPending_ = false;
}

void App::requestConversationExitFromInteraction(
    const __FlashStringHelper* source) {
  if (!voiceConversationActive_ && !conversationFollowUpPending_) {
    return;
  }

  conversationFollowUpPending_ = false;
  const bool requested = realtimeVoiceService_.requestConversationExit();
  Serial.print(F("conversation exit request source="));
  Serial.print(source);
  Serial.print(F(" ok="));
  Serial.println(requested ? "1" : "0");
}

void App::handleConversationWindow() {
  if (realtimeVoiceService_.consumeConversationEndedEvent()) {
    voiceConversationActive_ = false;
    conversationFollowUpPending_ = false;
    Serial.println(F("conversation ended source=server_goodbye state=idle"));
  }

  if (realtimeVoiceService_.consumeConversationTimeoutEvent()) {
    voiceConversationActive_ = false;
    conversationFollowUpPending_ = false;
    Serial.println(F("conversation window timeout source=server_no_voice"));
    const ScenarioPlan plan = scenarioEngine_.select(
        {ScenarioEventType::ConversationWindowTimeout, 0}, scenarioContext_);
    scenarioExecutor_.execute(plan);
  }

  if (realtimeVoiceService_.consumeTurnCompletedEvent()) {
    if (voiceConversationActive_ &&
        !realtimeVoiceService_.snapshot().conversationExitRequested) {
      openConversationWindow();
    }
  }

  if (conversationFollowUpPending_ &&
      static_cast<long>(millis() - conversationFollowUpAtMs_) >= 0) {
    conversationFollowUpPending_ = false;
    startConversationVoiceTurn();
  }
}

void App::openConversationWindow() {
  conversationFollowUpPending_ = true;
  conversationFollowUpAtMs_ = millis() + kConversationPlaybackTailMs;
  Serial.print(F("conversation window open mode=immediate_upload t="));
  Serial.print(millis());
  Serial.print(F(" start_at="));
  Serial.println(conversationFollowUpAtMs_);
}

void App::closeConversationWindow(bool playExitCue) {
  if (!voiceConversationActive_ && !conversationFollowUpPending_) {
    return;
  }

  voiceConversationActive_ = false;
  conversationFollowUpPending_ = false;
  const RealtimeVoiceSnapshot voice = realtimeVoiceService_.snapshot();
  if (voice.state == RealtimeVoiceState::Listening ||
      voice.state == RealtimeVoiceState::AwaitingResponse) {
    realtimeVoiceService_.abortBackendSpeech();
  }
  Serial.print(F("conversation window close play_exit="));
  Serial.println(playExitCue ? "1" : "0");
  if (playExitCue) {
    const ScenarioPlan plan = scenarioEngine_.select(
        {ScenarioEventType::ConversationWindowTimeout, 0}, scenarioContext_);
    scenarioExecutor_.execute(plan);
  }
}

void App::handleReminderEvents() {
  ScenarioEvent event;
  if (!reminderManager_.update(event)) {
    return;
  }

  const ScenarioPlan plan = scenarioEngine_.select(event, scenarioContext_);
  scenarioExecutor_.execute(plan);
}

void App::handleAudioCueEvents() {
  for (uint8_t i = 0; i < 3; ++i) {
    const WebConfigEvent event = webConfigServer_.consumeEvent();
    if (event == WebConfigEvent::None) {
      break;
    }

    switch (event) {
      case WebConfigEvent::SetupPortalStarted:
        playLocalizedCue(LocalizedAudioCue::WifiConfig, "wifi_config");
        break;
      case WebConfigEvent::LocaleChanged:
        playLocalizedCue(LocalizedAudioCue::WifiConfig, "locale_changed_wifi_config");
        break;
      case WebConfigEvent::WifiConnected:
        playSystemCue(SystemAudioCue::Success, "wifi_connected");
        break;
      case WebConfigEvent::WifiConnectFailed:
        playSystemCue(SystemAudioCue::Error, "wifi_connect_failed");
        break;
      case WebConfigEvent::None:
      default:
        break;
    }
  }

  String activationCode;
  if (deviceActivationService_.consumeActivationCodeEvent(activationCode)) {
    playActivationCodeCue(activationCode);
  }
}

void App::handleRealtimeVoiceEvents() {
  if (!realtimeVoiceService_.consumeFailureEvent()) {
    return;
  }

  voiceConversationActive_ = false;
  conversationFollowUpPending_ = false;
  playSystemCue(SystemAudioCue::Error, "voice_recognition_failed");
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

void App::playSystemCue(SystemAudioCue cue, const char* label) {
  logAudioCueResult(label, hardware_.audioCuePlayer().playSystem(cue));
}

void App::playLocalizedCue(LocalizedAudioCue cue, const char* label) {
  logAudioCueResult(label, hardware_.audioCuePlayer().playLocalized(cue, deviceLocaleStore_.locale()));
}

void App::playActivationCodeCue(const String& code) {
  logAudioCueResult("activation_code",
                    hardware_.audioCuePlayer().playActivationCode(code, deviceLocaleStore_.locale()));
}

void App::logAudioCueResult(const char* label, const AudioCuePlaybackResult& result) const {
  Serial.print("audio cue ");
  Serial.print(label);
  Serial.print(" ok=");
  Serial.print(result.ok ? "1" : "0");
  Serial.print(" packets=");
  Serial.print(result.packets);
  Serial.print(" samples=");
  Serial.print(result.samples);
  if (!result.ok) {
    Serial.print(" error=");
    Serial.print(result.error);
  }
  Serial.println();
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
  Serial.print("  device locale=");
  Serial.println(deviceLocaleStore_.locale());
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
