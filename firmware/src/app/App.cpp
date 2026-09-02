#include "app/App.h"

#include <WiFi.h>

#include "tongdou/BuildInfo.h"

namespace tongdou {
namespace {

constexpr unsigned long kFirstStartupReportMs = 2500;
constexpr unsigned long kSecondStartupReportMs = 8000;
constexpr unsigned long kConversationPlaybackTailMs = 120;
constexpr unsigned long kLocaleResourceRepairRetryMs = 60000;
constexpr unsigned long kReminderCompletionCommitRetryMs = 1000;

}  // namespace

void App::begin() {
  Serial.begin(115200);
  bootMs_ = millis();
  hardware_.begin();

  audioVolumeService_.begin();
  Serial.print(F("audio volume ready="));
  Serial.print(audioVolumeService_.ready() ? "1" : "0");
  Serial.print(F(" percent="));
  Serial.println(audioVolumeService_.volume());

  delay(200);
  Serial.println();
  Serial.print(kProjectName);
  Serial.print(" firmware stage: ");
  Serial.println(kFirmwareStage);

  scenarioExecutor_.begin();
  Serial.println("scenario executor ready");

  localAudioPipeline_.begin();
  Serial.println("local audio pipeline ready");

  localWakeWordDetector_.begin();
  if (localWakeWordDetector_.snapshot().ready) {
    localAudioPipeline_.setMonitor(&localWakeWordDetector_);
  }

  realtimeVoiceService_.begin();
  realtimeVoiceService_.setMcpPayloadHandler(
      [this](const String& payload) {
        return mcpServer_.handleVoicePayloadText(payload);
      });
  voiceVisualFeedback_.begin();
  Serial.println("realtime voice ready");

  serverEndpointStore_.begin();
  Serial.print("server endpoint ready=");
  Serial.println(serverEndpointStore_.otaUrl().length() > 0 ? "1" : "0");

  deviceActivationService_.begin();
  Serial.println("device activation ready");

  wifiConfigStore_.begin();
  Serial.println("wifi config ready");

  deviceLocaleStore_.begin();
  Serial.print("device locale=");
  Serial.println(deviceLocaleStore_.locale());

  localeResourceStore_.begin();
  hardware_.audioCuePlayer().useLocaleResourceStore(localeResourceStore_);
  Serial.print("locale resources ready=");
  Serial.print(localeResourceStore_.ready() ? "1" : "0");
  Serial.print(" locale=");
  Serial.print(localeResourceStore_.locale());
  Serial.print(" version=");
  Serial.print(localeResourceStore_.version());
  Serial.print(" error=");
  Serial.println(localeResourceStore_.lastError());

  timeService_.begin();
  Serial.println("time service started");

  reminderStore_.begin();
  Serial.println("reminder store ready");

  reminderManager_.begin();
  Serial.println("reminder manager ready");

  reminderAlertService_.begin();
  Serial.println("reminder alert service ready");

  proactiveReminderConfigStore_.begin();
  proactiveReminderService_.setSettings(proactiveReminderConfigStore_.settings());
  proactiveReminderService_.begin(millis());
  Serial.println("proactive reminder service ready");

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
  handleReminderAlertCompletion();
  handleRealtimeVoiceEvents();
  handleConversationWindow();
  updateLocaleResourceEndpoint();
  handleLocaleResourceRepair();
  deviceActivationService_.update();
  handleAudioCueEvents();
  handleStartupReport();
  handleSerialDiagnostics();
  handleLogoTouchEvents();
  handleLocalWakeWordEvents();
  handleReminderEvents();
  handleProactiveReminderEvents();
  updateWakeWordArming();
  scenarioExecutor_.update();
  voiceVisualFeedback_.update(reminderAlertService_.blocksVoiceInteraction());
  reminderAlertService_.update();
}

void App::updateLocaleResourceEndpoint() {
  const unsigned long nowMs = millis();
  if (static_cast<long>(nowMs - nextLocaleResourceEndpointSyncMs_) < 0) {
    return;
  }

  nextLocaleResourceEndpointSyncMs_ = nowMs + 1000;
  const RealtimeVoiceSnapshot voice = realtimeVoiceService_.snapshot();
  const String oldBaseUrl = localeResourceUpdateService_.baseUrl();
  const uint16_t oldFormatVersion =
      localeResourceUpdateService_.formatVersion();
  localeResourceUpdateService_.setEndpoint(
      voice.backendLocaleResourceBaseUrl,
      voice.backendLocaleResourceFormatVersion);
  if (oldBaseUrl != localeResourceUpdateService_.baseUrl() ||
      oldFormatVersion != localeResourceUpdateService_.formatVersion()) {
    nextLocaleResourceRepairMs_ = 0;
  }
}

void App::handleLocaleResourceRepair() {
  const String locale = deviceLocaleStore_.locale();
  if (localeResourceStore_.matchesLocale(locale)) {
    nextLocaleResourceRepairMs_ = 0;
    return;
  }

  const unsigned long nowMs = millis();
  if (nextLocaleResourceRepairMs_ != 0 &&
      static_cast<long>(nowMs - nextLocaleResourceRepairMs_) < 0) {
    return;
  }

  const RealtimeVoiceSnapshot voice = realtimeVoiceService_.snapshot();
  if (WiFi.status() != WL_CONNECTED || !voice.backendReady ||
      voice.state != RealtimeVoiceState::Idle || voiceConversationActive_ ||
      conversationFollowUpPending_ || reminderResponseStartPending_ ||
      !hardware_.audioOutput().streamDrained() ||
      localeResourceUpdateService_.baseUrl().length() == 0 ||
      localeResourceUpdateService_.formatVersion() == 0) {
    return;
  }

  nextLocaleResourceRepairMs_ = nowMs + kLocaleResourceRepairRetryMs;
  Serial.print(F("locale repair start locale="));
  Serial.print(locale);
  Serial.print(F(" current="));
  Serial.print(localeResourceStore_.locale());
  Serial.print(F(" error="));
  Serial.println(localeResourceStore_.lastError());
  const LocaleResourceUpdateResult result =
      localeResourceUpdateService_.downloadAndInstall(locale);
  Serial.print(F("locale repair done ok="));
  Serial.print(result.ok ? "1" : "0");
  Serial.print(F(" bytes="));
  Serial.print(result.downloadedBytes);
  if (!result.ok) {
    Serial.print(F(" error="));
    Serial.print(result.error);
  }
  Serial.println();
  if (result.ok) {
    nextLocaleResourceRepairMs_ = 0;
  }
}

void App::handleLocalWakeWordEvents() {
  if (!localWakeWordDetector_.consumeDetectedEvent()) {
    return;
  }

  if (reminderAlertService_.blocksVoiceInteraction()) {
    Serial.println(F("voice wake ignored reason=reminder_announcement_active"));
    return;
  }
  reminderAlertService_.dismissDisplayedReminder();

  closeConversationWindow(false);
  constexpr char kWakeText[] = "你好，铜豆";
  const bool started = realtimeVoiceService_.startWakeGreeting(kWakeText);
  Serial.print(F("voice wake start source=local_word ok="));
  Serial.println(started ? "1" : "0");
  voiceConversationActive_ = started;
  conversationFollowUpPending_ = false;
  if (!started) {
    playSystemCue(SystemAudioCue::Error, "local_wake_backend_failed");
  }
}

void App::updateWakeWordArming() {
  const RealtimeVoiceSnapshot voice = realtimeVoiceService_.snapshot();
  const LocalWakeWordSnapshot wake = localWakeWordDetector_.snapshot();
  const bool interactionIdle =
      voice.state == RealtimeVoiceState::Idle &&
      !voiceConversationActive_ &&
      !conversationFollowUpPending_ &&
      !reminderResponseStartPending_ &&
      !reminderAlertService_.blocksVoiceInteraction();

  if (!startupVoiceReady_) {
    localWakeWordDetector_.setArmed(false);

    if (!startupReadyCueStarted_) {
      if (!voice.backendReady || !wake.ready || !interactionIdle ||
          !hardware_.audioOutput().streamDrained()) {
        return;
      }

      Serial.print(F("startup timeline voice_backend_ready elapsed_ms="));
      Serial.print(millis() - bootMs_);
      Serial.println(F(" cue=success"));
      playSystemCue(SystemAudioCue::Success, "voice_ready");
      startupReadyCueStarted_ = true;
      return;
    }

    if (!voice.backendReady || !wake.ready || !interactionIdle ||
        !hardware_.audioOutput().streamDrained()) {
      return;
    }

    startupVoiceReady_ = true;
    Serial.print(F("startup timeline wake_ready elapsed_ms="));
    Serial.println(millis() - bootMs_);
  }

  const bool shouldArm = startupVoiceReady_ && voice.backendReady &&
                         interactionIdle;
  localWakeWordDetector_.setArmed(shouldArm);
}

void App::handleLogoTouchEvents() {
  const LogoTouchEvent touchEvent = hardware_.logoTouch().consumeEvent();
  if (touchEvent == LogoTouchEvent::None) {
    return;
  }

  if (reminderAlertService_.blocksVoiceInteraction()) {
    Serial.println(F("logo event ignored reason=reminder_announcement_active"));
    return;
  }
  reminderAlertService_.dismissDisplayedReminder();

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

void App::startReminderResponseVoiceTurn() {
  localWakeWordDetector_.setArmed(false);
  Serial.print(F("voice timeline app_start t="));
  Serial.print(millis());
  Serial.print(F(" source=reminder_response reminder="));
  Serial.print(reminderResponseId_);
  Serial.println(F(" pre_roll_samples=0"));

  const bool started = realtimeVoiceService_.startReminderResponseTurn();
  const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
  Serial.print(F("reminder response start id="));
  Serial.print(reminderResponseId_);
  Serial.print(F(" interrupted_conversation="));
  Serial.print(reminderInterruptedConversation_ ? "1" : "0");
  Serial.print(F(" ok="));
  Serial.print(started ? "1" : "0");
  Serial.print(F(" code="));
  Serial.println(started ? String("ok") : snapshot.lastErrorCode);

  reminderResponseStartPending_ = false;
  reminderInterruptedConversation_ = false;
  voiceConversationActive_ = started;
  if (!started) {
    reminderResponseId_ = 0;
    mcpServer_.clearReminderResponseContext();
  }
}

void App::startVoiceTurnFromInteraction(const __FlashStringHelper* source) {
  localWakeWordDetector_.setArmed(false);
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
  if (!voiceConversationActive_ && !conversationFollowUpPending_ &&
      !reminderResponseStartPending_) {
    return;
  }

  conversationFollowUpPending_ = false;
  reminderResponseStartPending_ = false;
  reminderResponseId_ = 0;
  mcpServer_.clearReminderResponseContext();
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
    reminderResponseStartPending_ = false;
    reminderResponseId_ = 0;
    mcpServer_.clearReminderResponseContext();
    Serial.println(F("conversation ended source=server_goodbye state=idle"));
  }

  if (realtimeVoiceService_.consumeConversationTimeoutEvent()) {
    voiceConversationActive_ = false;
    conversationFollowUpPending_ = false;
    reminderResponseId_ = 0;
    mcpServer_.clearReminderResponseContext();
    Serial.println(F("conversation window timeout source=server_no_voice"));
    const ScenarioPlan plan = scenarioEngine_.select(
        {ScenarioEventType::ConversationWindowTimeout, 0}, scenarioContext_);
    scenarioExecutor_.execute(plan);
  }

  if (realtimeVoiceService_.consumeReminderResponseTimeoutEvent()) {
    voiceConversationActive_ = false;
    conversationFollowUpPending_ = false;
    reminderResponseStartPending_ = false;
    Serial.print(F("reminder response closed reason=no_voice id="));
    Serial.println(reminderResponseId_);
    reminderResponseId_ = 0;
    mcpServer_.clearReminderResponseContext();
  }

  if (realtimeVoiceService_.consumeTurnCompletedEvent()) {
    if (voiceConversationActive_ &&
        !realtimeVoiceService_.snapshot().conversationExitRequested) {
      openConversationWindow();
    }
  }

  if (reminderResponseStartPending_ &&
      static_cast<long>(millis() - reminderResponseStartAtMs_) >= 0) {
    startReminderResponseVoiceTurn();
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
  if (!voiceConversationActive_ && !conversationFollowUpPending_ &&
      !reminderResponseStartPending_) {
    return;
  }

  voiceConversationActive_ = false;
  conversationFollowUpPending_ = false;
  reminderResponseStartPending_ = false;
  reminderResponseId_ = 0;
  mcpServer_.clearReminderResponseContext();
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

void App::handleReminderAlertCompletion() {
  if (reminderCompletionCommitPendingId_ == 0) {
    ReminderAlertCompletion completion;
    if (!reminderAlertService_.consumeCompletedEvent(completion)) {
      return;
    }
    if (completion.proactiveType != ProactiveReminderType::None) {
      scenarioExecutor_.finishActivePlan();
      proactiveReminderService_.delivered(completion.proactiveType, millis());
      Serial.print(F("proactive reminder delivered type="));
      Serial.println(static_cast<uint8_t>(completion.proactiveType));
      return;
    }
    reminderCompletionCommitPendingId_ = completion.reminderId;
    reminderCompletionOnline_ = completion.online;
    reminderCompletionCommitRetryAtMs_ = 0;
    reminderCompletionCommitFailureLogged_ = false;
  }

  const unsigned long nowMs = millis();
  if (reminderCompletionCommitRetryAtMs_ != 0 &&
      static_cast<long>(nowMs - reminderCompletionCommitRetryAtMs_) < 0) {
    return;
  }

  if (!reminderManager_.markDelivered(
          reminderCompletionCommitPendingId_)) {
    if (!reminderCompletionCommitFailureLogged_) {
      Serial.print(F("reminder completion commit failed id="));
      Serial.println(reminderCompletionCommitPendingId_);
      reminderCompletionCommitFailureLogged_ = true;
    }
    reminderCompletionCommitRetryAtMs_ =
        nowMs + kReminderCompletionCommitRetryMs;
    return;
  }

  const uint32_t completedId = reminderCompletionCommitPendingId_;
  const bool completedOnline = reminderCompletionOnline_;
  reminderCompletionCommitPendingId_ = 0;
  reminderCompletionOnline_ = false;
  reminderCompletionCommitRetryAtMs_ = 0;
  reminderCompletionCommitFailureLogged_ = false;

  const RealtimeVoiceSnapshot voice = realtimeVoiceService_.snapshot();
  if (!completedOnline || !voice.backendReady ||
      voice.state != RealtimeVoiceState::Idle) {
    Serial.print(F("reminder response unavailable id="));
    Serial.print(completedId);
    Serial.print(F(" online="));
    Serial.print(completedOnline ? "1" : "0");
    Serial.print(F(" backend_ready="));
    Serial.println(voice.backendReady ? "1" : "0");
    reminderInterruptedConversation_ = false;
    return;
  }

  reminderResponseId_ = completedId;
  mcpServer_.setReminderResponseContext(completedId);
  reminderResponseStartPending_ = true;
  reminderResponseStartAtMs_ = millis() + kConversationPlaybackTailMs;
  Serial.print(F("reminder response scheduled id="));
  Serial.print(completedId);
  Serial.print(F(" start_at="));
  Serial.println(reminderResponseStartAtMs_);
}

void App::handleReminderEvents() {
  if (reminderCompletionCommitPendingId_ != 0) {
    return;
  }
  if (!reminderAlertService_.canStart()) {
    return;
  }

  ReminderRecord dueReminder;
  if (!reminderManager_.update(dueReminder)) {
    return;
  }

  reminderInterruptedConversation_ =
      voiceConversationActive_ || conversationFollowUpPending_ ||
      reminderResponseStartPending_;
  closeConversationWindow(false);
  const RadarSnapshot radar = hardware_.radar().read();
  Serial.print(F("reminder delivery context id="));
  Serial.print(dueReminder.id);
  Serial.print(F(" event="));
  Serial.print(dueReminder.eventId);
  Serial.print(F(" kind="));
  Serial.print(reminderOccurrenceKindName(dueReminder.kind));
  Serial.print(F(" radar_occupied="));
  Serial.println(radar.occupied ? "1" : "0");
  const ScenarioPlan plan = scenarioEngine_.select(
      {ScenarioEventType::ReminderDue, 0}, scenarioContext_);
  scenarioExecutor_.execute(plan);
  if (!reminderAlertService_.start(dueReminder, deviceLocaleStore_.locale())) {
    Serial.print(F("reminder alert start failed id="));
    Serial.println(dueReminder.id);
  }
}

void App::handleProactiveReminderEvents() {
  const unsigned long nowMs = millis();
  proactiveReminderService_.setSettings(proactiveReminderConfigStore_.settings());
  proactiveReminderService_.update(nowMs, hardware_.radar().read().occupied);

  const RealtimeVoiceSnapshot voice = realtimeVoiceService_.snapshot();
  const bool deliveryAvailable =
      reminderCompletionCommitPendingId_ == 0 && reminderAlertService_.canStart() &&
      voice.state == RealtimeVoiceState::Idle && !voiceConversationActive_ &&
      !conversationFollowUpPending_ && !reminderResponseStartPending_;
  const ProactiveReminderType type =
      proactiveReminderService_.takeDue(nowMs, deliveryAvailable);
  if (type == ProactiveReminderType::None) {
    return;
  }

  const ScenarioEventType scenarioEvent =
      type == ProactiveReminderType::ActivityBreak
          ? ScenarioEventType::UserIdleTooLong
          : ScenarioEventType::HydrationReminderDue;
  scenarioExecutor_.execute(scenarioEngine_.select({scenarioEvent, 0}, scenarioContext_));
  if (!reminderAlertService_.startProactive(type, deviceLocaleStore_.locale())) {
    proactiveReminderService_.release(type);
    Serial.println(F("proactive reminder alert start failed"));
  }
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
        Serial.print(F("startup timeline wifi_ready elapsed_ms="));
        Serial.print(millis() - bootMs_);
        Serial.println(F(" waiting_voice_backend=1"));
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
  reminderResponseStartPending_ = false;
  reminderResponseId_ = 0;
  reminderInterruptedConversation_ = false;
  mcpServer_.clearReminderResponseContext();
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

  if (trimmed == F("wake") || trimmed == F("wake status")) {
    localWakeWordDetector_.printDiagnostics(Serial);
    return;
  }

  if (trimmed == F("locale resources")) {
    Serial.println(F("locale resources:"));
    Serial.print(F("  ready="));
    Serial.println(localeResourceStore_.ready() ? "1" : "0");
    Serial.print(F("  locale="));
    Serial.println(localeResourceStore_.locale());
    Serial.print(F("  version="));
    Serial.println(localeResourceStore_.version());
    Serial.print(F("  error="));
    Serial.println(localeResourceStore_.lastError());
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
  const LocalAudioPipelineSnapshot localAudio = localAudioPipeline_.snapshot();
  Serial.print("  local audio ready=");
  Serial.print(localAudio.ready ? "1" : "0");
  Serial.print(" samples_read=");
  Serial.print(localAudio.samplesRead);
  Serial.print(" upload_active=");
  Serial.println(localAudio.uploadActive ? "1" : "0");
  const LocalWakeWordSnapshot wake = localWakeWordDetector_.snapshot();
  Serial.print("  local wake ready=");
  Serial.print(wake.ready ? "1" : "0");
  Serial.print(" armed=");
  Serial.print(wake.armed ? "1" : "0");
  Serial.print(" phrase=你好，铜豆 frame_samples=");
  Serial.print(wake.modelFrameSamples);
  Serial.print(" input_gain=");
  Serial.print(wake.inputGain);
  Serial.print(" psram_queue_bytes=");
  Serial.print(wake.psramUsedBytes);
  Serial.print(" samples_offered=");
  Serial.print(wake.samplesOffered);
  Serial.print(" samples_dropped=");
  Serial.print(wake.samplesDropped);
  Serial.print(" detections=");
  Serial.print(wake.detections);
  if (wake.lastError[0] != '\0') {
    Serial.print(" error=");
    Serial.print(wake.lastError);
  }
  Serial.println();
  Serial.println("  device activation ready");
  Serial.println("  wifi config ready");
  Serial.print("  device locale=");
  Serial.println(deviceLocaleStore_.locale());
  Serial.print("  locale resources ready=");
  Serial.print(localeResourceStore_.ready() ? "1" : "0");
  Serial.print(" locale=");
  Serial.print(localeResourceStore_.locale());
  Serial.print(" version=");
  Serial.print(localeResourceStore_.version());
  Serial.print(" error=");
  Serial.println(localeResourceStore_.lastError());
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
