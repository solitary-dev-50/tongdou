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
                                     RealtimeVoiceService& realtimeVoiceService)
    : scenarioEngine_(scenarioEngine),
      scenarioExecutor_(scenarioExecutor),
      scenarioContext_(scenarioContext),
      realtimeVoiceService_(realtimeVoiceService) {}

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

AppCommandResult AppCommandService::startVoiceTurn() {
  if (!realtimeVoiceService_.startTurn()) {
    return {false, "voice_start_failed", "voice turn failed to start"};
  }
  return {true, "ok", "voice turn started"};
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
  doc["lastErrorCode"] = snapshot.lastErrorCode;
  doc["lastErrorMessage"] = snapshot.lastErrorMessage;

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
