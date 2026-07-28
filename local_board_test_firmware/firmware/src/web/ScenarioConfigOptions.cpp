#include "web/ScenarioConfigOptions.h"

namespace tongdou {
namespace {

constexpr PersonalityStyle kPersonalities[] = {
    PersonalityStyle::Gentle,
    PersonalityStyle::Balanced,
    PersonalityStyle::Dramatic,
};

constexpr ScenarioEventType kEvents[] = {
    ScenarioEventType::BootCompleted,
    ScenarioEventType::UserArrived,
    ScenarioEventType::UserLeft,
    ScenarioEventType::UserIdleTooLong,
    ScenarioEventType::ReminderDue,
    ScenarioEventType::ReminderConfirmed,
    ScenarioEventType::ReminderSnoozed,
    ScenarioEventType::QuietModeRequested,
    ScenarioEventType::LowBattery,
    ScenarioEventType::ChargingStarted,
    ScenarioEventType::OvertimeReminderDue,
    ScenarioEventType::VoiceRecognitionFailed,
};

constexpr FaceAction kFaces[] = {
    FaceAction::None,      FaceAction::Sleep,    FaceAction::WakeUp,
    FaceAction::Awake,     FaceAction::Blink,    FaceAction::Smile,
    FaceAction::Serious,   FaceAction::RollEyes, FaceAction::Sleepy,
    FaceAction::Wronged,   FaceAction::Squint,   FaceAction::Innocent,
    FaceAction::Confused,  FaceAction::Angry,    FaceAction::Surprised,
    FaceAction::Shy,       FaceAction::Fierce,   FaceAction::Proud,
    FaceAction::Nervous,
};

constexpr LightAction kLights[] = {
    LightAction::None,        LightAction::Off,          LightAction::SoftWhite,
    LightAction::WarmWake,    LightAction::RedShortBlink, LightAction::WeakBreath,
    LightAction::DimWarm,
};

constexpr MotionAction kMotions[] = {
    MotionAction::None,
    MotionAction::Stop,
    MotionAction::Nod,
    MotionAction::TinyShake,
    MotionAction::NudgeForward,
    MotionAction::LeanForward,
    MotionAction::ShrinkBack,
};

constexpr VoiceLine kVoices[] = {
    VoiceLine::None,
    VoiceLine::WakeGreeting,
    VoiceLine::SnoozeSoft,
    VoiceLine::SnoozeTease,
    VoiceLine::ConfirmReward,
    VoiceLine::QuietReply,
    VoiceLine::BreakReminder,
    VoiceLine::OvertimeNudge,
    VoiceLine::OvertimeCowardReply,
    VoiceLine::ReminderDueNudge,
    VoiceLine::LowBatteryWhine,
    VoiceLine::ChargingRelief,
    VoiceLine::RecognitionCoffeeChoke,
};

template <typename T, size_t N>
const T* expose(const T (&items)[N], size_t& count) {
  count = N;
  return items;
}

}  // namespace

const PersonalityStyle* ScenarioConfigOptions::personalities(size_t& count) const {
  return expose(kPersonalities, count);
}

const ScenarioEventType* ScenarioConfigOptions::events(size_t& count) const {
  return expose(kEvents, count);
}

const FaceAction* ScenarioConfigOptions::faces(size_t& count) const {
  return expose(kFaces, count);
}

const LightAction* ScenarioConfigOptions::lights(size_t& count) const {
  return expose(kLights, count);
}

const MotionAction* ScenarioConfigOptions::motions(size_t& count) const {
  return expose(kMotions, count);
}

const VoiceLine* ScenarioConfigOptions::voices(size_t& count) const {
  return expose(kVoices, count);
}

}  // namespace tongdou
