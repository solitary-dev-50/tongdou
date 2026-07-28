#include "scenario/ScenarioLibrary.h"

namespace tongdou {
namespace {

constexpr ScenarioRule kRules[] = {
    {
        ScenarioEventType::BootCompleted,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::WakeUp, LightAction::Off, MotionAction::None,
         VoiceLine::None, 1200},
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
        {true, FaceAction::Fierce, LightAction::SoftWhite,
         MotionAction::NudgeForward, VoiceLine::ReminderDueNudge, 1200},
    },
    {
        ScenarioEventType::ReminderConfirmed,
        false,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Proud, LightAction::SoftWhite, MotionAction::Nod,
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
        {true, FaceAction::Shy, LightAction::WarmWake, MotionAction::TinyShake,
         VoiceLine::ChargingRelief, 1000},
    },
    {
        ScenarioEventType::VoiceRecognitionFailed,
        true,
        true,
        PersonalityStyle::Gentle,
        {true, FaceAction::Nervous, LightAction::RedShortBlink,
         MotionAction::ShrinkBack, VoiceLine::RecognitionCoffeeChoke, 1200},
    },
    {
        ScenarioEventType::OvertimeReminderDue,
        false,
        true,
        PersonalityStyle::Balanced,
        {true, FaceAction::Squint, LightAction::DimWarm, MotionAction::LeanForward,
         VoiceLine::OvertimeNudge, 1800},
    },
    {
        ScenarioEventType::DanceShowRequested,
        false,
        false,
        PersonalityStyle::Dramatic,
        {true, FaceAction::Proud, LightAction::WeakBreath, MotionAction::CheekyDance,
         VoiceLine::DanceShowDomestic, 1800},
    },
    {
        ScenarioEventType::DanceShowGlobalRequested,
        false,
        false,
        PersonalityStyle::Dramatic,
        {true, FaceAction::Proud, LightAction::WeakBreath, MotionAction::CheekyDance,
         VoiceLine::DanceShowGlobal, 1800},
    },
};

constexpr ScenarioPack kDefaultPack = {
    {kScenarioPackFormatVersion, "tongdou.default.v1", "Default Tong Dou",
     "First built-in pack for warm, cheeky desk companion behavior.",
     PersonalityStyle::Balanced},
    kRules,
    sizeof(kRules) / sizeof(kRules[0]),
};

}  // namespace

const ScenarioPack& ScenarioLibrary::pack() const {
  return kDefaultPack;
}

const ScenarioRule* ScenarioLibrary::rules() const {
  return kDefaultPack.rules;
}

size_t ScenarioLibrary::size() const {
  return kDefaultPack.ruleCount;
}

}  // namespace tongdou
