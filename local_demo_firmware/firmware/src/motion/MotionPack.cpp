#include "motion/MotionPack.h"

namespace tongdou {
namespace {

constexpr MotionFrame kStop[] = {
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kNod[] = {
    {WheelDrive::Forward, WheelDrive::Forward, 80},
    {WheelDrive::Reverse, WheelDrive::Reverse, 80},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kTinyShake[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 70},
    {WheelDrive::Reverse, WheelDrive::Forward, 70},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kNudgeForward[] = {
    {WheelDrive::Forward, WheelDrive::Forward, 100},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kLeanForward[] = {
    {WheelDrive::Forward, WheelDrive::Forward, 140},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kShrinkBack[] = {
    {WheelDrive::Reverse, WheelDrive::Reverse, 120},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kCheekyDance[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 160},
    {WheelDrive::Stop, WheelDrive::Stop, 80},
    {WheelDrive::Reverse, WheelDrive::Forward, 160},
    {WheelDrive::Stop, WheelDrive::Stop, 80},
    {WheelDrive::Forward, WheelDrive::Reverse, 140},
    {WheelDrive::Reverse, WheelDrive::Forward, 140},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kCheekyDanceV2[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 120, 150},
    {WheelDrive::Forward, WheelDrive::Reverse, 420, 200},
    {WheelDrive::Forward, WheelDrive::Reverse, 140, 145},
    {WheelDrive::Stop, WheelDrive::Stop, 100},
    {WheelDrive::Forward, WheelDrive::Reverse, 220, 165},
    {WheelDrive::Stop, WheelDrive::Stop, 120},
    {WheelDrive::Reverse, WheelDrive::Forward, 220, 165},
    {WheelDrive::Stop, WheelDrive::Stop, 120},
    {WheelDrive::Forward, WheelDrive::Reverse, 210, 170},
    {WheelDrive::Stop, WheelDrive::Stop, 80},
    {WheelDrive::Reverse, WheelDrive::Forward, 210, 170},
    {WheelDrive::Stop, WheelDrive::Stop, 80},
    {WheelDrive::Forward, WheelDrive::Reverse, 180, 175},
    {WheelDrive::Reverse, WheelDrive::Forward, 180, 175},
    {WheelDrive::Brake, WheelDrive::Brake, 110, 150},
    {WheelDrive::Stop, WheelDrive::Stop, 380},
    {WheelDrive::Reverse, WheelDrive::Forward, 120, 145},
    {WheelDrive::Reverse, WheelDrive::Forward, 420, 190},
    {WheelDrive::Reverse, WheelDrive::Forward, 160, 135},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kTinyReluctantShake[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 170, 125},
    {WheelDrive::Stop, WheelDrive::Stop, 140},
    {WheelDrive::Reverse, WheelDrive::Forward, 170, 125},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kTurnAwayToRear[] = {
    {WheelDrive::Forward, WheelDrive::Stop, 220, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 140},
    {WheelDrive::Forward, WheelDrive::Stop, 260, 200},
    {WheelDrive::Stop, WheelDrive::Stop, 160},
    {WheelDrive::Forward, WheelDrive::Stop, 180, 170},
    {WheelDrive::Stop, WheelDrive::Stop, 220},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kRearWiggleLeft[] = {
    {WheelDrive::Forward, WheelDrive::Stop, 230, 160},
    {WheelDrive::Stop, WheelDrive::Stop, 130},
    {WheelDrive::Reverse, WheelDrive::Stop, 160, 125},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kRearWiggleRight[] = {
    {WheelDrive::Stop, WheelDrive::Forward, 230, 160},
    {WheelDrive::Stop, WheelDrive::Stop, 130},
    {WheelDrive::Stop, WheelDrive::Reverse, 160, 125},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kRearWiggleShowoff[] = {
    {WheelDrive::Forward, WheelDrive::Stop, 240, 165},
    {WheelDrive::Stop, WheelDrive::Stop, 120},
    {WheelDrive::Stop, WheelDrive::Forward, 240, 165},
    {WheelDrive::Stop, WheelDrive::Stop, 120},
    {WheelDrive::Forward, WheelDrive::Stop, 220, 175},
    {WheelDrive::Stop, WheelDrive::Stop, 120},
    {WheelDrive::Stop, WheelDrive::Forward, 220, 175},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kHardBrakeStop[] = {
    {WheelDrive::Brake, WheelDrive::Brake, 110, 150},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kTurnBackToFront[] = {
    {WheelDrive::Reverse, WheelDrive::Stop, 220, 175},
    {WheelDrive::Stop, WheelDrive::Stop, 140},
    {WheelDrive::Reverse, WheelDrive::Stop, 220, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 180},
    {WheelDrive::Reverse, WheelDrive::Stop, 140, 155},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonSpinBrake[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 240},
    {WheelDrive::Reverse, WheelDrive::Forward, 220},
    {WheelDrive::Forward, WheelDrive::Reverse, 180},
    {WheelDrive::Brake, WheelDrive::Brake, 120},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};

constexpr uint8_t kSummonLeftPwm = 187;
constexpr uint8_t kSummonRightPwm = 170;
constexpr uint8_t kSummonBrakePwm = 170;
constexpr uint8_t kSummonStartLeftPwm = 224;
constexpr uint8_t kSummonStartRightPwm = 198;
constexpr uint16_t kSummonStartLungeMs = 240;
constexpr uint16_t kSummonStretchBackMs = 170;
constexpr uint16_t kSummonSmallTurnMs = 90;
constexpr uint16_t kSummonReturnTurnMs = 85;
constexpr uint16_t kSummonCreepForwardMs = 160;
constexpr uint16_t kSummonThreatNudgeMs = 70;
constexpr uint16_t kSummonThreatRestMs = 280;
constexpr uint16_t kSummonGoodLungeMs = 220;
constexpr uint16_t kSummonHoldTiltMs = 100;
constexpr uint16_t kSummonHoldThreatMs = 55;
constexpr uint16_t kSummonHoldThreatRestMs = 240;
constexpr uint16_t kSummonHoldReturnMs = 95;
constexpr uint16_t kSummonNightmarePulseMs = 100;
constexpr uint16_t kSummonNightmareRestMs = 150;
constexpr uint16_t kSummonNowLungeMs = 300;
constexpr uint16_t kSummonLaughTurnMs = 85;
constexpr uint16_t kSummonLaughRestMs = 80;
constexpr uint16_t kSummonBrakeMs = 80;

constexpr MotionFrame kSummonStartLunge[] = {
    {WheelDrive::Forward, WheelDrive::Forward, kSummonStartLungeMs, 0,
     kSummonStartLeftPwm, kSummonStartRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonStretchBack[] = {
    {WheelDrive::Reverse, WheelDrive::Reverse, kSummonStretchBackMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonLookAside[] = {
    {WheelDrive::Forward, WheelDrive::Stop, kSummonSmallTurnMs, 0,
     kSummonLeftPwm, 0},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonReturnFromLook[] = {
    {WheelDrive::Reverse, WheelDrive::Stop, kSummonReturnTurnMs, 0,
     kSummonLeftPwm, 0},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonCreepForward[] = {
    {WheelDrive::Forward, WheelDrive::Forward, kSummonCreepForwardMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonThreatRestMs},
    {WheelDrive::Forward, WheelDrive::Forward, kSummonThreatNudgeMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 60, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonThreatRestMs},
    {WheelDrive::Forward, WheelDrive::Forward, kSummonThreatNudgeMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 60, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonGoodLunge[] = {
    {WheelDrive::Forward, WheelDrive::Forward, kSummonGoodLungeMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonHoldTilt[] = {
    {WheelDrive::Stop, WheelDrive::Forward, kSummonHoldTiltMs, 0,
     0, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonHoldThreatRestMs},
    {WheelDrive::Stop, WheelDrive::Forward, kSummonHoldThreatMs, 0,
     0, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 60, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonReturnFromHold[] = {
    {WheelDrive::Stop, WheelDrive::Reverse, kSummonHoldReturnMs, 0,
     0, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonNightmareCreep[] = {
    {WheelDrive::Forward, WheelDrive::Forward, kSummonNightmarePulseMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonNightmareRestMs},
    {WheelDrive::Forward, WheelDrive::Forward, kSummonNightmarePulseMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonNightmareRestMs},
    {WheelDrive::Forward, WheelDrive::Forward, kSummonNightmarePulseMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonNowLunge[] = {
    {WheelDrive::Forward, WheelDrive::Forward, kSummonNowLungeMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 110, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonLaughWiggle[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, kSummonLaughTurnMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 60, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonLaughRestMs},
    {WheelDrive::Reverse, WheelDrive::Forward, kSummonLaughTurnMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 60, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonLaughRestMs},
    {WheelDrive::Forward, WheelDrive::Reverse, kSummonLaughTurnMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, 60, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, kSummonLaughRestMs},
    {WheelDrive::Reverse, WheelDrive::Forward, kSummonLaughTurnMs, 0,
     kSummonLeftPwm, kSummonRightPwm},
    {WheelDrive::Brake, WheelDrive::Brake, kSummonBrakeMs, 0,
     kSummonBrakePwm, kSummonBrakePwm},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kAccountantConfusedFidget[] = {
    {WheelDrive::Reverse, WheelDrive::Reverse, 170, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 180},
    {WheelDrive::Forward, WheelDrive::Forward, 160, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kAccountantExtortLean[] = {
    {WheelDrive::Forward, WheelDrive::Forward, 220, 190},
    {WheelDrive::Stop, WheelDrive::Stop, 300},
    {WheelDrive::Reverse, WheelDrive::Reverse, 180, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kAccountantStubbornWiggle[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 190, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 170},
    {WheelDrive::Reverse, WheelDrive::Forward, 190, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 170},
    {WheelDrive::Forward, WheelDrive::Reverse, 160, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 140},
    {WheelDrive::Reverse, WheelDrive::Forward, 160, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kTinySmugWiggle[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 180, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 180},
    {WheelDrive::Reverse, WheelDrive::Forward, 180, 185},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};

template <size_t Count>
MotionSequence makeSequence(const MotionFrame (&frames)[Count]) {
  return {frames, Count};
}

}  // namespace

MotionSequence MotionPack::sequence(MotionAction action) const {
  switch (action) {
    case MotionAction::Stop:
      return makeSequence(kStop);
    case MotionAction::Nod:
      return makeSequence(kNod);
    case MotionAction::TinyShake:
      return makeSequence(kTinyShake);
    case MotionAction::NudgeForward:
      return makeSequence(kNudgeForward);
    case MotionAction::LeanForward:
      return makeSequence(kLeanForward);
    case MotionAction::ShrinkBack:
      return makeSequence(kShrinkBack);
    case MotionAction::CheekyDance:
      return makeSequence(kCheekyDance);
    case MotionAction::CheekyDanceV2:
      return makeSequence(kCheekyDanceV2);
    case MotionAction::TinyReluctantShake:
      return makeSequence(kTinyReluctantShake);
    case MotionAction::TurnAwayToRear:
      return makeSequence(kTurnAwayToRear);
    case MotionAction::RearWiggleLeft:
      return makeSequence(kRearWiggleLeft);
    case MotionAction::RearWiggleRight:
      return makeSequence(kRearWiggleRight);
    case MotionAction::RearWiggleShowoff:
      return makeSequence(kRearWiggleShowoff);
    case MotionAction::HardBrakeStop:
      return makeSequence(kHardBrakeStop);
    case MotionAction::TurnBackToFront:
      return makeSequence(kTurnBackToFront);
    case MotionAction::SummonSpinBrake:
      return makeSequence(kSummonSpinBrake);
    case MotionAction::SummonStartLunge:
      return makeSequence(kSummonStartLunge);
    case MotionAction::SummonStretchBack:
      return makeSequence(kSummonStretchBack);
    case MotionAction::SummonLookAside:
      return makeSequence(kSummonLookAside);
    case MotionAction::SummonReturnFromLook:
      return makeSequence(kSummonReturnFromLook);
    case MotionAction::SummonCreepForward:
      return makeSequence(kSummonCreepForward);
    case MotionAction::SummonGoodLunge:
      return makeSequence(kSummonGoodLunge);
    case MotionAction::SummonHoldTilt:
      return makeSequence(kSummonHoldTilt);
    case MotionAction::SummonReturnFromHold:
      return makeSequence(kSummonReturnFromHold);
    case MotionAction::SummonNightmareCreep:
      return makeSequence(kSummonNightmareCreep);
    case MotionAction::SummonNowLunge:
      return makeSequence(kSummonNowLunge);
    case MotionAction::SummonLaughWiggle:
      return makeSequence(kSummonLaughWiggle);
    case MotionAction::AccountantConfusedFidget:
      return makeSequence(kAccountantConfusedFidget);
    case MotionAction::AccountantExtortLean:
      return makeSequence(kAccountantExtortLean);
    case MotionAction::AccountantStubbornWiggle:
      return makeSequence(kAccountantStubbornWiggle);
    case MotionAction::TinySmugWiggle:
      return makeSequence(kTinySmugWiggle);
    case MotionAction::None:
    default:
      return {};
  }
}

}  // namespace tongdou
