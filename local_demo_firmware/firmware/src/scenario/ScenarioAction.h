#pragma once

#include <stdint.h>

namespace tongdou {

enum class FaceAction : uint8_t {
  None,
  Sleep,
  WakeUp,
  Awake,
  Blink,
  Smile,
  Serious,
  RollEyes,
  Sleepy,
  Wronged,
  Squint,
  Innocent,
  Confused,
  Angry,
  Surprised,
  Shy,
  Fierce,
  Proud,
  Nervous,
};

enum class LightAction : uint8_t {
  None,
  Off,
  SoftWhite,
  WarmWake,
  RedShortBlink,
  WeakBreath,
  DimWarm,
  DimCyan,
};

enum class MotionAction : uint8_t {
  None,
  Stop,
  Nod,
  TinyShake,
  NudgeForward,
  LeanForward,
  ShrinkBack,
  CheekyDance,
  SummonSpinBrake,
};

enum class VoiceLine : uint8_t {
  None,
  WakeGreeting,
  SnoozeSoft,
  SnoozeTease,
  ConfirmReward,
  QuietReply,
  BreakReminder,
  OvertimeNudge,
  OvertimeCowardReply,
  ReminderDueNudge,
  LowBatteryWhine,
  ChargingRelief,
  RecognitionCoffeeChoke,
  DanceShowDomestic,
  DanceShowGlobal,
  DemoNightGossipGlobal,
  DemoNightCowardGlobal,
  DemoMorningSoftGlobal,
  DemoRecordedEmailSummaryGlobal,
  DemoFirstSummonNightmare,
  DemoCrowdfundingCallGlobal,
};

}  // namespace tongdou
