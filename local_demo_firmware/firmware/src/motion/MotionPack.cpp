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
    {WheelDrive::Forward, WheelDrive::Reverse, 180},
    {WheelDrive::Reverse, WheelDrive::Forward, 180},
    {WheelDrive::Forward, WheelDrive::Reverse, 170},
    {WheelDrive::Reverse, WheelDrive::Forward, 170},
    {WheelDrive::Forward, WheelDrive::Reverse, 160},
    {WheelDrive::Reverse, WheelDrive::Forward, 160},
    {WheelDrive::Forward, WheelDrive::Reverse, 150},
    {WheelDrive::Reverse, WheelDrive::Forward, 150},
    {WheelDrive::Brake, WheelDrive::Brake, 120},
    {WheelDrive::Stop, WheelDrive::Stop, 0},
};
constexpr MotionFrame kSummonSpinBrake[] = {
    {WheelDrive::Forward, WheelDrive::Reverse, 240},
    {WheelDrive::Reverse, WheelDrive::Forward, 220},
    {WheelDrive::Forward, WheelDrive::Reverse, 180},
    {WheelDrive::Brake, WheelDrive::Brake, 120},
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
    case MotionAction::SummonSpinBrake:
      return makeSequence(kSummonSpinBrake);
    case MotionAction::None:
    default:
      return {};
  }
}

}  // namespace tongdou
