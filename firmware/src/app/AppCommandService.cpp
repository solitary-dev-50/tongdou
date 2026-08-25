#include "app/AppCommandService.h"

#include <ArduinoJson.h>

namespace tongdou {
namespace {

String voiceStateName(RealtimeVoiceState state) {
  switch (state) {
    case RealtimeVoiceState::Listening:
      return "listening";
    case RealtimeVoiceState::AwaitingResponse:
      return "awaiting_response";
    case RealtimeVoiceState::Ending:
      return "ending";
    case RealtimeVoiceState::Failed:
      return "failed";
    case RealtimeVoiceState::Idle:
    default:
      return "idle";
  }
}

}  // namespace

AppCommandService::AppCommandService(ScenarioEngine& scenarioEngine,
                                     ScenarioExecutor& scenarioExecutor,
                                     ScenarioContext& scenarioContext,
                                     RealtimeVoiceService& realtimeVoiceService,
                                     AudioVolumeService& audioVolumeService)
    : scenarioEngine_(scenarioEngine),
      scenarioExecutor_(scenarioExecutor),
      scenarioContext_(scenarioContext),
      realtimeVoiceService_(realtimeVoiceService),
      audioVolumeService_(audioVolumeService) {}

AppCommandResult AppCommandService::playScenario(const String& eventName) {
  ScenarioEventType eventType;
  if (!parseEventName(eventName, eventType)) {
    return {false, "invalid_event", "unknown scenario event"};
  }

  const ScenarioPlan plan = scenarioEngine_.select({eventType, 0}, scenarioContext_);
  if (!plan.valid) {
    return {false, "scenario_not_found", "no scenario plan matched current context"};
  }

  scenarioExecutor_.execute(plan);
  return {true, "ok", "scenario accepted"};
}

AppCommandResult AppCommandService::setPersonality(const String& personalityNameValue) {
  PersonalityStyle style;
  if (!parsePersonalityName(personalityNameValue, style)) {
    return {false, "invalid_personality", "unknown personality"};
  }

  scenarioContext_.personality = style;
  return {true, "ok", "personality updated"};
}

AppCommandResult AppCommandService::configureVoiceBackend(const String& host, int port,
                                                          const String& path,
                                                          const String& token,
                                                          const String& deviceId,
                                                          const String& clientId,
                                                          bool useTls) {
  String safeHost = host;
  safeHost.trim();
  if (safeHost.length() == 0) {
    return {false, "invalid_voice_backend", "voice backend host is required"};
  }

  TongDouVoiceConnectionConfig config;
  config.host = safeHost;
  config.port = port > 0 ? static_cast<uint16_t>(port) : 8100;
  config.path = path.length() > 0 ? path : "/xiaozhi/v1/";
  config.authorization = token;
  config.deviceId = deviceId;
  config.clientId = clientId.length() > 0 ? clientId : "tongdou-demo";
  config.useTls = useTls;

  realtimeVoiceService_.configureBackend(config);
  return {true, "ok", "voice backend configured"};
}

AppCommandResult AppCommandService::connectVoiceBackend() {
  if (!realtimeVoiceService_.connectBackend()) {
    const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
    const String code = snapshot.lastErrorCode.length() > 0
                            ? snapshot.lastErrorCode
                            : String("voice_connect_failed");
    const String message = snapshot.lastErrorMessage.length() > 0
                               ? snapshot.lastErrorMessage
                               : String("voice backend failed to connect");
    return {false, code, message};
  }
  return {true, "ok", "voice backend connecting"};
}

AppCommandResult AppCommandService::sendVoiceDetect(const String& text) {
  String safeText = text;
  safeText.trim();
  if (safeText.length() == 0) {
    return {false, "invalid_voice_detect", "detect text is required"};
  }
  if (!realtimeVoiceService_.sendDetectText(safeText)) {
    return {false, "voice_detect_failed", "voice backend is not ready"};
  }
  return {true, "ok", "voice detect sent"};
}

AppCommandResult AppCommandService::abortVoice() {
  if (!realtimeVoiceService_.abortBackendSpeech()) {
    return {false, "voice_abort_failed", "voice backend is not connected"};
  }
  return {true, "ok", "voice abort sent"};
}

AppCommandResult AppCommandService::exitVoiceConversation() {
  if (!realtimeVoiceService_.requestConversationExit()) {
    const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
    return {false,
            snapshot.lastErrorCode.length() > 0
                ? snapshot.lastErrorCode
                : String("voice_exit_failed"),
            snapshot.lastErrorMessage.length() > 0
                ? snapshot.lastErrorMessage
                : String("voice conversation exit request failed")};
  }
  return {true, "ok", "voice conversation exit requested"};
}

AppCommandResult AppCommandService::startVoiceTurn(uint16_t captureMs) {
  if (!realtimeVoiceService_.startTurn(captureMs)) {
    return {false, "voice_start_failed", "voice turn failed to start"};
  }
  return {true, "ok", "voice turn started"};
}

AppCommandResult AppCommandService::startAutoVoiceTurn() {
  if (!realtimeVoiceService_.startAutoTurn()) {
    const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
    return {false,
            snapshot.lastErrorCode.length() > 0
                ? snapshot.lastErrorCode
                : String("voice_auto_start_failed"),
            snapshot.lastErrorMessage.length() > 0
                ? snapshot.lastErrorMessage
                : String("automatic voice turn failed to start")};
  }
  return {true, "ok", "automatic voice turn started"};
}

AppCommandResult AppCommandService::playVoiceResponse(const String& text) {
  if (text.length() == 0) {
    return {false, "invalid_response", "response text is required"};
  }
  if (!realtimeVoiceService_.playResponse(0, text)) {
    return {false, "voice_response_failed", "voice response failed to play"};
  }
  return {true, "ok", "voice response accepted"};
}

AppCommandResult AppCommandService::failVoiceTurn(const String& reason) {
  const String message = reason.length() == 0 ? "voice turn failed" : reason;
  realtimeVoiceService_.failTurn("voice_turn_failed", message);
  return {true, "ok", "voice failure accepted"};
}

AppCommandResult AppCommandService::setAudioVolume(int volumePercent) {
  if (volumePercent < 0 || volumePercent > 100) {
    return {false, "invalid_audio_volume", "volume must be between 0 and 100"};
  }
  if (!audioVolumeService_.setVolume(static_cast<uint8_t>(volumePercent))) {
    return {false, "audio_volume_save_failed", "audio volume could not be saved"};
  }
  return {true, "ok", "audio volume updated"};
}

AppCommandResult AppCommandService::adjustAudioVolume(int deltaPercent) {
  if (deltaPercent == 0 || deltaPercent < -100 || deltaPercent > 100) {
    return {false, "invalid_audio_volume_delta",
            "volume delta must be between -100 and 100 and cannot be zero"};
  }

  const int requested = static_cast<int>(audioVolumeService_.volume()) +
                        deltaPercent;
  const int clamped = max(0, min(100, requested));
  return setAudioVolume(clamped);
}

String AppCommandService::voiceStatusJson() const {
  const RealtimeVoiceSnapshot snapshot = realtimeVoiceService_.snapshot();
  JsonDocument doc;
  doc["ok"] = true;
  doc["state"] = voiceStateName(snapshot.state);
  doc["turnId"] = snapshot.turnId;
  doc["audioInputReady"] = snapshot.audioInputReady;
  doc["audioOutputReady"] = snapshot.audioOutputReady;
  doc["lastTurnOk"] = snapshot.lastTurnOk;
  doc["chunksSent"] = snapshot.chunksSent;
  doc["samplesSent"] = snapshot.samplesSent;
  doc["bytesSent"] = snapshot.bytesSent;
  doc["backend"]["state"] = snapshot.backendState;
  doc["backend"]["connected"] = snapshot.backendConnected;
  doc["backend"]["ready"] = snapshot.backendReady;
  doc["backend"]["sessionId"] = snapshot.backendSessionId;
  doc["backend"]["localeResourceBaseUrl"] =
      snapshot.backendLocaleResourceBaseUrl;
  doc["backend"]["localeResourceFormatVersion"] =
      snapshot.backendLocaleResourceFormatVersion;
  doc["backend"]["textMessagesReceived"] = snapshot.backendTextMessagesReceived;
  doc["backend"]["binaryPacketsReceived"] = snapshot.backendBinaryPacketsReceived;
  doc["backend"]["mcpRequestsReceived"] = snapshot.backendMcpRequestsReceived;
  doc["backend"]["mcpResponsesSent"] = snapshot.backendMcpResponsesSent;
  doc["backend"]["mcpFailures"] = snapshot.backendMcpFailures;
  doc["backend"]["uplinkPcmFramesEncoded"] = snapshot.backendUplinkPcmFramesEncoded;
  doc["backend"]["uplinkOpusPacketsSent"] = snapshot.backendUplinkOpusPacketsSent;
  doc["backend"]["uplinkEncodeFailures"] = snapshot.backendUplinkEncodeFailures;
  doc["backend"]["downlinkPacketsPlayed"] = snapshot.backendDownlinkPacketsPlayed;
  doc["backend"]["downlinkPlaybackFailures"] = snapshot.backendDownlinkPlaybackFailures;
  doc["backend"]["downlinkQueued"] = snapshot.backendDownlinkQueued;
  doc["backend"]["downlinkPushed"] = snapshot.backendDownlinkPushed;
  doc["backend"]["downlinkPopped"] = snapshot.backendDownlinkPopped;
  doc["backend"]["downlinkDropped"] = snapshot.backendDownlinkDropped;
  doc["backend"]["downlinkOversized"] = snapshot.backendDownlinkOversized;
  doc["backend"]["codecReady"] = snapshot.backendCodecReady;
  doc["backend"]["codecEncodeAttempts"] = snapshot.backendCodecEncodeAttempts;
  doc["backend"]["codecDecodeAttempts"] = snapshot.backendCodecDecodeAttempts;
  doc["backend"]["codecEncodeFailures"] = snapshot.backendCodecEncodeFailures;
  doc["backend"]["codecDecodeFailures"] = snapshot.backendCodecDecodeFailures;
  doc["backend"]["codecNativeError"] = snapshot.backendCodecNativeError;
  doc["backend"]["codecNativeErrorText"] = snapshot.backendCodecNativeErrorText;
  doc["backend"]["codecLastError"] = snapshot.backendCodecLastError;
  doc["backend"]["lastMessageType"] = snapshot.backendLastMessageType;
  doc["backend"]["lastTtsState"] = snapshot.backendLastTtsState;
  doc["backend"]["lastText"] = snapshot.backendLastText;
  doc["backend"]["lastEmotion"] = snapshot.backendLastEmotion;
  doc["backend"]["lastError"] = snapshot.backendLastError;
  doc["conversation"]["exitRequested"] = snapshot.conversationExitRequested;
  doc["conversation"]["goodbyeReceived"] = snapshot.backendGoodbyeReceived;
  doc["conversation"]["goodbyeReason"] = snapshot.backendGoodbyeReason;
  doc["visual"]["speechOutputActive"] = snapshot.speechOutputActive;
  doc["visual"]["reminderAnnouncementActive"] =
      snapshot.reminderAnnouncementActive;
  doc["lastErrorCode"] = snapshot.lastErrorCode;
  doc["lastErrorMessage"] = snapshot.lastErrorMessage;

  String output;
  serializeJson(doc, output);
  return output;
}

String AppCommandService::voiceCodecSelfTestJson() {
  const TongDouVoiceAudioCodecSelfTestResult result =
      realtimeVoiceService_.runCodecSelfTest();

  JsonDocument doc;
  doc["ok"] = result.ok;
  doc["inputSamples"] = result.inputSamples;
  doc["encodedBytes"] = result.encodedBytes;
  doc["decodedSamples"] = result.decodedSamples;
  doc["inputMeanAbs"] = result.inputMeanAbs;
  doc["decodedMeanAbs"] = result.decodedMeanAbs;
  doc["nativeError"] = result.nativeError;
  doc["nativeErrorText"] = result.nativeErrorText;
  doc["error"] = result.error;

  String output;
  serializeJson(doc, output);
  return output;
}

const char* AppCommandService::personalityName() const {
  switch (scenarioContext_.personality) {
    case PersonalityStyle::Gentle:
      return "gentle";
    case PersonalityStyle::Dramatic:
      return "dramatic";
    case PersonalityStyle::Balanced:
    default:
      return "balanced";
  }
}

uint8_t AppCommandService::audioVolume() const {
  return audioVolumeService_.volume();
}

bool AppCommandService::parseEventName(String name, ScenarioEventType& eventType) const {
  name.trim();
  name.toLowerCase();

  if (name == F("boot_completed")) {
    eventType = ScenarioEventType::BootCompleted;
  } else if (name == F("user_arrived")) {
    eventType = ScenarioEventType::UserArrived;
  } else if (name == F("user_left")) {
    eventType = ScenarioEventType::UserLeft;
  } else if (name == F("user_idle_too_long")) {
    eventType = ScenarioEventType::UserIdleTooLong;
  } else if (name == F("reminder_due")) {
    eventType = ScenarioEventType::ReminderDue;
  } else if (name == F("reminder_confirmed")) {
    eventType = ScenarioEventType::ReminderConfirmed;
  } else if (name == F("reminder_snoozed")) {
    eventType = ScenarioEventType::ReminderSnoozed;
  } else if (name == F("quiet_mode_requested")) {
    eventType = ScenarioEventType::QuietModeRequested;
  } else if (name == F("low_battery")) {
    eventType = ScenarioEventType::LowBattery;
  } else if (name == F("charging_started")) {
    eventType = ScenarioEventType::ChargingStarted;
  } else if (name == F("overtime_reminder_due")) {
    eventType = ScenarioEventType::OvertimeReminderDue;
  } else if (name == F("voice_recognition_failed")) {
    eventType = ScenarioEventType::VoiceRecognitionFailed;
  } else if (name == F("conversation_window_timeout")) {
    eventType = ScenarioEventType::ConversationWindowTimeout;
  } else if (name == F("dance_show")) {
    eventType = ScenarioEventType::DanceShowRequested;
  } else if (name == F("dance_show_global")) {
    eventType = ScenarioEventType::DanceShowGlobalRequested;
  } else if (name == F("logo_single_tapped")) {
    eventType = ScenarioEventType::LogoSingleTapped;
  } else if (name == F("logo_double_tapped")) {
    eventType = ScenarioEventType::LogoDoubleTapped;
  } else if (name == F("logo_long_pressed")) {
    eventType = ScenarioEventType::LogoLongPressed;
  } else {
    return false;
  }

  return true;
}

bool AppCommandService::parsePersonalityName(String name, PersonalityStyle& style) const {
  name.trim();
  name.toLowerCase();

  if (name == F("gentle")) {
    style = PersonalityStyle::Gentle;
  } else if (name == F("balanced")) {
    style = PersonalityStyle::Balanced;
  } else if (name == F("dramatic")) {
    style = PersonalityStyle::Dramatic;
  } else {
    return false;
  }

  return true;
}

}  // namespace tongdou
