#include "demo/DemoSceneLibrary.h"

namespace tongdou {
namespace {

constexpr ScenarioPlan plan(FaceAction face, LightAction light, MotionAction motion,
                            VoiceLine voice, uint16_t durationMs) {
  return {true, face, light, motion, voice, durationMs};
}

constexpr DemoSceneStep kLateNightDoghouseHook[] = {
    {0, plan(FaceAction::Squint, LightAction::DimWarm, MotionAction::None,
             VoiceLine::None, 300)},
    {300, plan(FaceAction::Squint, LightAction::DimWarm, MotionAction::LeanForward,
               VoiceLine::None, 400)},
    {700, plan(FaceAction::Fierce, LightAction::DimWarm, MotionAction::None,
               VoiceLine::DemoNightGossipGlobal, 2900)},
    {3600, plan(FaceAction::Proud, LightAction::WeakBreath, MotionAction::None,
                VoiceLine::None, 1000)},
};

constexpr DemoSceneStep kFirstSummonNightmare[] = {
    {0, plan(FaceAction::Sleep, LightAction::Off, MotionAction::None,
             VoiceLine::None, 300)},
    {300, plan(FaceAction::Sleep, LightAction::DimWarm, MotionAction::None,
               VoiceLine::None, 400)},
    {700, plan(FaceAction::WakeUp, LightAction::WeakBreath, MotionAction::None,
               VoiceLine::None, 400)},
    {1100, plan(FaceAction::Surprised, LightAction::WarmWake, MotionAction::TinyShake,
                VoiceLine::None, 400)},
    {1500, plan(FaceAction::Fierce, LightAction::DimWarm, MotionAction::None,
                VoiceLine::DemoFirstSummonNightmare, 1100)},
    {2600, plan(FaceAction::Fierce, LightAction::DimWarm, MotionAction::SummonSpinBrake,
                VoiceLine::None, 800)},
    {3400, plan(FaceAction::Squint, LightAction::DimCyan, MotionAction::None,
                VoiceLine::None, 1800)},
    {5200, plan(FaceAction::Proud, LightAction::WeakBreath, MotionAction::Stop,
                VoiceLine::None, 1200)},
};

constexpr DemoSceneStep kLateNightCowardTail[] = {
    {0, plan(FaceAction::Surprised, LightAction::DimWarm, MotionAction::TinyShake,
             VoiceLine::None, 300)},
    {300, plan(FaceAction::Innocent, LightAction::WeakBreath, MotionAction::ShrinkBack,
               VoiceLine::DemoNightCowardGlobal, 2400)},
    {2700, plan(FaceAction::Sleepy, LightAction::WeakBreath, MotionAction::None,
                VoiceLine::None, 800)},
};

constexpr DemoSceneStep kMorningWakeup[] = {
    {0, plan(FaceAction::Sleep, LightAction::Off, MotionAction::None,
             VoiceLine::None, 400)},
    {400, plan(FaceAction::WakeUp, LightAction::WarmWake, MotionAction::TinyShake,
               VoiceLine::None, 800)},
    {1200, plan(FaceAction::Awake, LightAction::SoftWhite, MotionAction::Nod,
                VoiceLine::DemoMorningSoftGlobal, 2000)},
    {3200, plan(FaceAction::Smile, LightAction::SoftWhite, MotionAction::None,
                VoiceLine::None, 1000)},
};

constexpr DemoSceneStep kEmailSilentNotice[] = {
    {0, plan(FaceAction::Serious, LightAction::SoftWhite, MotionAction::None,
             VoiceLine::None, 800)},
    {800, plan(FaceAction::Serious, LightAction::WeakBreath, MotionAction::None,
               VoiceLine::None, 1800)},
};

constexpr DemoSceneStep kRealtimeEmailSummary[] = {
    {0, plan(FaceAction::Serious, LightAction::SoftWhite, MotionAction::None,
             VoiceLine::None, 300)},
    {300, plan(FaceAction::Awake, LightAction::SoftWhite, MotionAction::Nod,
               VoiceLine::DemoRecordedEmailSummaryGlobal, 3600)},
    {3900, plan(FaceAction::Smile, LightAction::WeakBreath, MotionAction::None,
                VoiceLine::None, 1000)},
};

constexpr DemoSceneStep kCheekyDanceGlobal[] = {
    {0, plan(FaceAction::Proud, LightAction::DimCyan, MotionAction::None,
             VoiceLine::DemoCheekyCalibrationDance, 500)},
    {500, plan(FaceAction::Proud, LightAction::SoftWhite, MotionAction::TinyShake,
               VoiceLine::None, 900)},
    {1400, plan(FaceAction::Squint, LightAction::DimCyan, MotionAction::CheekyDanceV2,
                VoiceLine::None, 1200)},
    {2600, plan(FaceAction::Proud, LightAction::WeakBreath, MotionAction::Stop,
                VoiceLine::None, 1000)},
    {3600, plan(FaceAction::Proud, LightAction::WeakBreath, MotionAction::None,
                VoiceLine::None, 700)},
    {4300, plan(FaceAction::Smile, LightAction::WeakBreath, MotionAction::Stop,
                VoiceLine::None, 1000)},
};

constexpr DemoSceneStep kCrowdfundingCallGlobal[] = {
    {0, plan(FaceAction::Proud, LightAction::DimWarm, MotionAction::TinyShake,
             VoiceLine::DemoCrowdfundingCallGlobal, 2600)},
    {2600, plan(FaceAction::Smile, LightAction::WeakBreath, MotionAction::Nod,
                VoiceLine::None, 1000)},
};

template <size_t Count>
DemoScene makeScene(DemoSceneId id, const DemoSceneStep (&steps)[Count],
                    bool startsRealtimeVoice = false) {
  return {id, steps, Count, startsRealtimeVoice};
}

}  // namespace

const char* demoSceneSlug(DemoSceneId id) {
  switch (id) {
    case DemoSceneId::LateNightDoghouseHook:
      return "late_night_doghouse_hook";
    case DemoSceneId::FirstSummonNightmare:
      return "first_summon_nightmare";
    case DemoSceneId::LateNightCowardTail:
      return "late_night_coward_tail_global";
    case DemoSceneId::MorningWakeup:
      return "morning_wakeup";
    case DemoSceneId::EmailSilentNotice:
      return "email_silent_notice";
    case DemoSceneId::RealtimeEmailSummary:
      return "realtime_email_summary";
    case DemoSceneId::CheekyDanceGlobal:
      return "cheeky_dance_global";
    case DemoSceneId::CrowdfundingCallGlobal:
      return "crowdfunding_call_global";
    case DemoSceneId::IdleStop:
    default:
      return "idle_stop";
  }
}

const char* demoSceneTitle(DemoSceneId id) {
  switch (id) {
    case DemoSceneId::LateNightDoghouseHook:
      return "Late-night doghouse hook";
    case DemoSceneId::FirstSummonNightmare:
      return "First Summon Nightmare";
    case DemoSceneId::LateNightCowardTail:
      return "Cowardly soft landing";
    case DemoSceneId::MorningWakeup:
      return "Morning wakeup";
    case DemoSceneId::EmailSilentNotice:
      return "Silent email notice";
    case DemoSceneId::RealtimeEmailSummary:
      return "Recorded email summary";
    case DemoSceneId::CheekyDanceGlobal:
      return "Cheeky dance";
    case DemoSceneId::CrowdfundingCallGlobal:
      return "Campaign call";
    case DemoSceneId::IdleStop:
    default:
      return "Idle";
  }
}

DemoSceneId demoSceneFromNumber(uint8_t number) {
  if (number <= 8) {
    return static_cast<DemoSceneId>(number);
  }
  return DemoSceneId::IdleStop;
}

DemoScene DemoSceneLibrary::scene(DemoSceneId id) const {
  switch (id) {
    case DemoSceneId::LateNightDoghouseHook:
      return makeScene(id, kLateNightDoghouseHook);
    case DemoSceneId::FirstSummonNightmare:
      return makeScene(id, kFirstSummonNightmare);
    case DemoSceneId::LateNightCowardTail:
      return makeScene(id, kLateNightCowardTail);
    case DemoSceneId::MorningWakeup:
      return makeScene(id, kMorningWakeup);
    case DemoSceneId::EmailSilentNotice:
      return makeScene(id, kEmailSilentNotice);
    case DemoSceneId::RealtimeEmailSummary:
      return makeScene(id, kRealtimeEmailSummary);
    case DemoSceneId::CheekyDanceGlobal:
      return makeScene(id, kCheekyDanceGlobal);
    case DemoSceneId::CrowdfundingCallGlobal:
      return makeScene(id, kCrowdfundingCallGlobal);
    case DemoSceneId::IdleStop:
    default:
      return {};
  }
}

}  // namespace tongdou
