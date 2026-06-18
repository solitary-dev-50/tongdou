#include "scenario/ScenarioLibrary.h"

namespace tongdou {
namespace {

constexpr ScenarioRule kRules[] = {
    {
        ScenarioEventType::BootCompleted,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::WakeUp, LightAction::WarmWake, MotionAction::TinyShake,
         VoiceLine::WakeGreeting, 1200},
    },
    {
        ScenarioEventType::ReminderSnoozed,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::RollEyes, LightAction::SoftWhite, MotionAction::None,
         VoiceLine::SnoozeSoft, 1000},
    },
    {
        ScenarioEventType::ReminderDue,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Serious, LightAction::SoftWhite,
         MotionAction::NudgeForward, VoiceLine::ReminderDueNudge, 1200},
    },
    {
        ScenarioEventType::ReminderConfirmed,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Smile, LightAction::SoftWhite, MotionAction::Nod,
         VoiceLine::ConfirmReward, 1000},
    },
    {
        ScenarioEventType::QuietModeRequested,
        true,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Wronged, LightAction::Off, MotionAction::Stop,
         VoiceLine::QuietReply, 800},
    },
    {
        ScenarioEventType::UserIdleTooLong,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Serious, LightAction::WeakBreath, MotionAction::None,
         VoiceLine::BreakReminder, 1200},
    },
    {
        ScenarioEventType::LowBattery,
        true,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Wronged, LightAction::RedShortBlink,
         MotionAction::ShrinkBack, VoiceLine::LowBatteryWhine, 1000},
    },
    {
        ScenarioEventType::ChargingStarted,
        true,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Smile, LightAction::WarmWake, MotionAction::TinyShake,
         VoiceLine::ChargingRelief, 1000},
    },
    {
        ScenarioEventType::OvertimeReminderDue,
        false,
        true,
        PersonalityStyle::Balanced,
        {true, FaceAction::Squint, LightAction::DimWarm, MotionAction::LeanForward,
         VoiceLine::OvertimeNudge, 1800},
    },
};

}  // namespace

const ScenarioRule* ScenarioLibrary::rules() const {
  return kRules;
}

size_t ScenarioLibrary::size() const {
  return sizeof(kRules) / sizeof(kRules[0]);
}

}  // namespace tongdou
