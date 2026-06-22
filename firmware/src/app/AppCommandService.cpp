#include "app/AppCommandService.h"

namespace tongdou {

AppCommandService::AppCommandService(ScenarioEngine& scenarioEngine,
                                     ScenarioExecutor& scenarioExecutor,
                                     ScenarioContext& scenarioContext)
    : scenarioEngine_(scenarioEngine),
      scenarioExecutor_(scenarioExecutor),
      scenarioContext_(scenarioContext) {}

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
