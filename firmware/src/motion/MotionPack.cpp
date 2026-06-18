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
    case MotionAction::None:
    default:
      return {};
  }
}

}  // namespace tongdou
