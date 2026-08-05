#include "demo/DemoSceneLibrary.h"

namespace tongdou {
namespace {

constexpr ScenarioPlan plan(FaceAction face, LightAction light, MotionAction motion,
                            VoiceLine voice, uint16_t durationMs) {
  return {true, face, light, motion, voice, durationMs};
}

constexpr DemoSceneStep kAccountantListeningPrelude[] = {
    {0, plan(FaceAction::Awake, LightAction::Off, MotionAction::Stop,
             VoiceLine::None, 1200)},
    {1200, plan(FaceAction::Blink, LightAction::Off, MotionAction::None,
                VoiceLine::None, 1400)},
    {2600, plan(FaceAction::RollEyes, LightAction::Off, MotionAction::None,
                VoiceLine::None, 1000)},
    {3600, plan(FaceAction::Awake, LightAction::Off,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 1600)},
    {5200, plan(FaceAction::Blink, LightAction::Off, MotionAction::None,
                VoiceLine::None, 1300)},
    {6500, plan(FaceAction::RollEyes, LightAction::Off, MotionAction::None,
                VoiceLine::None, 1000)},
    {7500, plan(FaceAction::Awake, LightAction::Off,
                MotionAction::TinySmugWiggle, VoiceLine::None, 1300)},
    {8800, plan(FaceAction::Awake, LightAction::Off, MotionAction::None,
                VoiceLine::None, 700)},
    {9500, plan(FaceAction::Awake, LightAction::Off, MotionAction::Stop,
                VoiceLine::None, 500)},
};

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
             VoiceLine::None, 800)},
    {800, plan(FaceAction::Sleep, LightAction::Off, MotionAction::None,
               VoiceLine::DemoFirstSummonNightmare, 0)},
    {0, plan(FaceAction::Sleep, LightAction::SummonNowFlash,
             MotionAction::SummonStartLunge, VoiceLine::None, 900)},
    {900, plan(FaceAction::WakeUp, LightAction::SummonCyanHold,
               MotionAction::Stop, VoiceLine::None, 370)},
    {1270, plan(FaceAction::FirstSummonConfusedHold, LightAction::SummonCyanFocus,
                MotionAction::SummonLookAside, VoiceLine::None, 1030)},
    {2300, plan(FaceAction::Squint, LightAction::SummonPurpleApproach,
                MotionAction::SummonCreepForward, VoiceLine::None, 1810)},
    {4110, plan(FaceAction::Squint, LightAction::SummonPurpleHold,
                MotionAction::SummonReturnFromLook, VoiceLine::None, 470)},
    {4580, plan(FaceAction::Proud, LightAction::SummonGoodFlash,
                MotionAction::SummonGoodLunge, VoiceLine::None, 980)},
    {5560, plan(FaceAction::Proud, LightAction::SummonPurpleHold,
                MotionAction::SummonHoldTilt, VoiceLine::None, 940)},
    {6500, plan(FaceAction::Proud, LightAction::SummonPurpleHold,
                MotionAction::SummonReturnFromHold, VoiceLine::None, 440)},
    {6940, plan(FaceAction::Squint, LightAction::SummonNightmareRise,
                MotionAction::SummonNightmareCreep, VoiceLine::None, 2140)},
    {9080, plan(FaceAction::Squint, LightAction::SummonDrop,
                MotionAction::Stop, VoiceLine::None, 1120)},
    {10200, plan(FaceAction::Fierce, LightAction::SummonNowFlash,
                 MotionAction::SummonNowLunge, VoiceLine::None, 970)},
    {11170, plan(FaceAction::Proud, LightAction::SummonLaughBreath,
                 MotionAction::SummonLaughWiggle, VoiceLine::None, 980)},
    {12150, plan(FaceAction::Proud, LightAction::SummonIdleCyan,
                 MotionAction::Stop, VoiceLine::None, 800)},
    {12950, plan(FaceAction::Awake, LightAction::Off,
                 MotionAction::Stop, VoiceLine::None, 500)},
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

constexpr DemoSceneStep kConfusedAccountantGlobal[] = {
    {0, plan(FaceAction::AccountantPause, LightAction::DimWarm, MotionAction::None,
             VoiceLine::DemoConfusedAccountant, 400)},
    {400, plan(FaceAction::AccountantRoundEyes, LightAction::DimWarm,
               MotionAction::None, VoiceLine::None, 400)},
    {800, plan(FaceAction::AccountantSuspiciousSquint, LightAction::DimWarm,
               MotionAction::None, VoiceLine::None, 1000)},
    {1800, plan(FaceAction::AccountantCalculating, LightAction::DimWarm,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 3000)},
    {4800, plan(FaceAction::AccountantLedgerSearch, LightAction::SoftWhite,
                MotionAction::None, VoiceLine::None, 2800)},
    {7600, plan(FaceAction::AccountantRoundEyes, LightAction::SoftWhite,
                MotionAction::None, VoiceLine::None, 600)},
    {8200, plan(FaceAction::AccountantDebt3, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 400)},
    {8600, plan(FaceAction::AccountantDebtScratch, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 1000)},
    {9600, plan(FaceAction::AccountantDebt2, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 1400)},
    {11000, plan(FaceAction::AccountantSuspiciousSquint, LightAction::WeakBreath,
                 MotionAction::AccountantExtortLean, VoiceLine::None, 2500)},
    {13500, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
                 MotionAction::AccountantConfusedFidget, VoiceLine::None, 1000)},
    {14500, plan(FaceAction::AccountantDebtPi, LightAction::DimWarm,
                 MotionAction::AccountantStubbornWiggle, VoiceLine::None, 3000)},
    {17500, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::TinySmugWiggle, VoiceLine::None, 1000)},
    {18500, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::Stop, VoiceLine::None, 800)},
    {19300, plan(FaceAction::Awake, LightAction::Off,
                 MotionAction::Stop, VoiceLine::None, 500)},
};

constexpr DemoSceneStep kAccountantClearDebt[] = {
    {0, plan(FaceAction::AccountantDebt3, LightAction::DimWarm,
             MotionAction::None, VoiceLine::DemoAccountantClearDebt, 1700)},
    {1700, plan(FaceAction::AccountantDebtScratch, LightAction::DimWarm,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 2200)},
    {3900, plan(FaceAction::AccountantDebt2, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 2700)},
    {6600, plan(FaceAction::AccountantSuspiciousSquint, LightAction::WeakBreath,
                MotionAction::AccountantExtortLean, VoiceLine::None, 4000)},
    {10600, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
                 MotionAction::AccountantConfusedFidget, VoiceLine::None, 3400)},
    {14000, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::TinySmugWiggle, VoiceLine::None, 3600)},
    {17600, plan(FaceAction::Awake, LightAction::Off,
                MotionAction::Stop, VoiceLine::None, 500)},
};

constexpr DemoSceneStep kAccountantEmotionalDamage[] = {
    {0, plan(FaceAction::AccountantRoundEyes, LightAction::DimWarm,
             MotionAction::None, VoiceLine::DemoAccountantEmotionalDamage, 800)},
    {800, plan(FaceAction::AccountantSuspiciousSquint, LightAction::DimWarm,
               MotionAction::AccountantConfusedFidget, VoiceLine::None, 2500)},
    {3300, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                MotionAction::AccountantExtortLean, VoiceLine::None, 2300)},
    {5600, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 1500)},
    {7100, plan(FaceAction::AccountantDebt3, LightAction::DimWarm,
                MotionAction::AccountantStubbornWiggle, VoiceLine::None, 3500)},
    {10600, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::AccountantExtortLean, VoiceLine::None, 2500)},
    {13100, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::TinySmugWiggle, VoiceLine::None, 1050)},
    {14150, plan(FaceAction::Awake, LightAction::Off,
                MotionAction::Stop, VoiceLine::None, 500)},
};

constexpr DemoSceneStep kAccountantBribeAccepted[] = {
    {0, plan(FaceAction::AccountantRoundEyes, LightAction::DimWarm,
             MotionAction::None, VoiceLine::DemoAccountantBribeAccepted, 900)},
    {900, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
               MotionAction::AccountantConfusedFidget, VoiceLine::None, 2050)},
    {2950, plan(FaceAction::AccountantDebt3, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 850)},
    {3800, plan(FaceAction::AccountantDebtScratch, LightAction::DimWarm,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 1800)},
    {5600, plan(FaceAction::AccountantDebt2, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 1000)},
    {6600, plan(FaceAction::AccountantSmugSquint, LightAction::DimWarm,
                MotionAction::AccountantExtortLean, VoiceLine::None, 1800)},
    {8400, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                MotionAction::TinySmugWiggle, VoiceLine::None, 1650)},
    {10050, plan(FaceAction::Awake, LightAction::Off,
                MotionAction::Stop, VoiceLine::None, 500)},
};

constexpr DemoSceneStep kAccountantSelectiveMemory[] = {
    {0, plan(FaceAction::AccountantLedgerSearch, LightAction::SoftWhite,
             MotionAction::None, VoiceLine::DemoAccountantSelectiveMemory, 700)},
    {700, plan(FaceAction::AccountantLedgerSearch, LightAction::SoftWhite,
               MotionAction::AccountantConfusedFidget, VoiceLine::None, 2100)},
    {2800, plan(FaceAction::AccountantDebt3, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 2300)},
    {5100, plan(FaceAction::AccountantRoundEyes, LightAction::DimWarm,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 2250)},
    {7350, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
                MotionAction::AccountantStubbornWiggle, VoiceLine::None, 2200)},
    {9550, plan(FaceAction::AccountantSmugSquint, LightAction::DimWarm,
                MotionAction::AccountantExtortLean, VoiceLine::None, 800)},
    {10350, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::TinySmugWiggle, VoiceLine::None, 900)},
    {11250, plan(FaceAction::Awake, LightAction::Off,
                MotionAction::Stop, VoiceLine::None, 500)},
};

constexpr DemoSceneStep kAccountantPiDebt[] = {
    {0, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
             MotionAction::None, VoiceLine::DemoAccountantPiDebt, 700)},
    {700, plan(FaceAction::AccountantCalculating, LightAction::WeakBreath,
               MotionAction::AccountantConfusedFidget, VoiceLine::None, 1750)},
    {2450, plan(FaceAction::AccountantDebtPi, LightAction::DimWarm,
                MotionAction::None, VoiceLine::None, 4400)},
    {6850, plan(FaceAction::AccountantDebtPi, LightAction::DimWarm,
                MotionAction::AccountantStubbornWiggle, VoiceLine::None, 1950)},
    {8800, plan(FaceAction::AccountantRoundEyes, LightAction::WeakBreath,
                MotionAction::AccountantConfusedFidget, VoiceLine::None, 3900)},
    {12700, plan(FaceAction::AccountantSmugSquint, LightAction::DimWarm,
                 MotionAction::AccountantExtortLean, VoiceLine::None, 2200)},
    {14900, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::TinySmugWiggle, VoiceLine::None, 2050)},
    {16950, plan(FaceAction::AccountantSmugSquint, LightAction::Off,
                 MotionAction::Stop, VoiceLine::None, 400)},
    {17350, plan(FaceAction::Awake, LightAction::Off,
                 MotionAction::Stop, VoiceLine::None, 1700)},
    {19050, plan(FaceAction::Blink, LightAction::Off,
                 MotionAction::None, VoiceLine::None, 1400)},
    {20450, plan(FaceAction::RollEyes, LightAction::Off,
                 MotionAction::None, VoiceLine::None, 1800)},
    {22250, plan(FaceAction::Awake, LightAction::Off,
                 MotionAction::None, VoiceLine::None, 1100)},
    {23350, plan(FaceAction::AccountantSmugSquint, LightAction::WeakBreath,
                 MotionAction::TinySmugWiggle, VoiceLine::None, 1400)},
    {24750, plan(FaceAction::AccountantSmugSquint, LightAction::Off,
                 MotionAction::Stop, VoiceLine::None, 1200)},
    {25950, plan(FaceAction::Awake, LightAction::Off,
                 MotionAction::Stop, VoiceLine::None, 500)},
};

constexpr DemoSceneStep kCrowdfundingCallGlobal[] = {
    {0, plan(FaceAction::Proud, LightAction::DimWarm, MotionAction::TinyShake,
             VoiceLine::DemoCrowdfundingCallGlobal, 2600)},
    {2600, plan(FaceAction::Smile, LightAction::WeakBreath, MotionAction::Nod,
                VoiceLine::None, 1000)},
};

template <size_t Count>
DemoScene makeScene(DemoSceneId id, const DemoSceneStep (&steps)[Count],
                    uint8_t variant = 0,
                    bool startsRealtimeVoice = false,
                    bool syncToVoiceStart = false) {
  return {id, variant, steps, Count, startsRealtimeVoice, syncToVoiceStart};
}

template <size_t Count, size_t PreludeCount>
DemoScene makeSceneWithPrelude(
    DemoSceneId id, const DemoSceneStep (&steps)[Count],
    const DemoSceneStep (&preludeSteps)[PreludeCount], uint8_t variant,
    bool syncToVoiceStart) {
  return {id, variant, steps, Count, false, syncToVoiceStart, preludeSteps,
          PreludeCount};
}

uint8_t normalizeAccountantVariant(uint8_t variant) {
  return variant >= 1 && variant <= 5 ? variant : 1;
}

}  // namespace

const char* demoSceneSlug(DemoSceneId id) {
  return demoSceneSlug(id, 0);
}

const char* demoSceneSlug(DemoSceneId id, uint8_t variant) {
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
    case DemoSceneId::ConfusedAccountantGlobal:
      switch (variant) {
        case 1:
          return "confused_accountant_clear";
        case 2:
          return "confused_accountant_extort";
        case 3:
          return "confused_accountant_bribe";
        case 4:
          return "confused_accountant_forgetful";
        case 5:
          return "confused_accountant_pi";
        default:
          return "confused_accountant_global";
      }
    case DemoSceneId::CrowdfundingCallGlobal:
      return "crowdfunding_call_global";
    case DemoSceneId::IdleStop:
    default:
      return "idle_stop";
  }
}

const char* demoSceneTitle(DemoSceneId id) {
  return demoSceneTitle(id, 0);
}

const char* demoSceneTitle(DemoSceneId id, uint8_t variant) {
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
    case DemoSceneId::ConfusedAccountantGlobal:
      switch (variant) {
        case 1:
          return "Scene 6-1: Clear debt";
        case 2:
          return "Scene 6-2: Emotional damage";
        case 3:
          return "Scene 6-3: Bribe accepted";
        case 4:
          return "Scene 6-4: Selective memory";
        case 5:
          return "Scene 6-5: Pi debt";
        default:
          return "Scene 6: Confused Accountant";
      }
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
  return scene(id, 0);
}

DemoScene DemoSceneLibrary::scene(DemoSceneId id, uint8_t variant) const {
  switch (id) {
    case DemoSceneId::LateNightDoghouseHook:
      return makeScene(id, kLateNightDoghouseHook);
    case DemoSceneId::FirstSummonNightmare:
      return makeScene(id, kFirstSummonNightmare, 0, false, true);
    case DemoSceneId::LateNightCowardTail:
      return makeScene(id, kLateNightCowardTail);
    case DemoSceneId::MorningWakeup:
      return makeScene(id, kMorningWakeup);
    case DemoSceneId::EmailSilentNotice:
      return makeScene(id, kEmailSilentNotice);
    case DemoSceneId::RealtimeEmailSummary:
      return makeScene(id, kRealtimeEmailSummary);
    case DemoSceneId::ConfusedAccountantGlobal: {
      const uint8_t selected = normalizeAccountantVariant(variant);
      switch (selected) {
        case 2:
          return makeSceneWithPrelude(id, kAccountantEmotionalDamage,
                                      kAccountantListeningPrelude, selected, true);
        case 3:
          return makeSceneWithPrelude(id, kAccountantBribeAccepted,
                                      kAccountantListeningPrelude, selected, true);
        case 4:
          return makeSceneWithPrelude(id, kAccountantSelectiveMemory,
                                      kAccountantListeningPrelude, selected, true);
        case 5:
          return makeSceneWithPrelude(id, kAccountantPiDebt,
                                      kAccountantListeningPrelude, selected, true);
        case 1:
        default:
          return makeSceneWithPrelude(id, kAccountantClearDebt,
                                      kAccountantListeningPrelude, selected, true);
      }
    }
    case DemoSceneId::CrowdfundingCallGlobal:
      return makeScene(id, kCrowdfundingCallGlobal);
    case DemoSceneId::IdleStop:
    default:
      return {};
  }
}

}  // namespace tongdou
