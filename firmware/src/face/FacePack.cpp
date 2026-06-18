#include "face/FacePack.h"

namespace tongdou {
namespace {

constexpr FaceFrame kNone[] = {
    {FaceExpression::Blank, 0},
};

constexpr FaceFrame kWakeUp[] = {
    {FaceExpression::Sleep, 180},
    {FaceExpression::HalfOpen, 220},
    {FaceExpression::Awake, 300},
    {FaceExpression::HalfOpen, 120},
    {FaceExpression::Awake, 380},
};

constexpr FaceFrame kSmile[] = {
    {FaceExpression::Awake, 160},
    {FaceExpression::Smile, 640},
};

constexpr FaceFrame kSerious[] = {
    {FaceExpression::Awake, 120},
    {FaceExpression::Serious, 880},
};

constexpr FaceFrame kRollEyes[] = {
    {FaceExpression::Serious, 120},
    {FaceExpression::RollEyesLeft, 240},
    {FaceExpression::RollEyesRight, 240},
    {FaceExpression::Awake, 260},
};

constexpr FaceFrame kSleepy[] = {
    {FaceExpression::HalfOpen, 300},
    {FaceExpression::Sleep, 700},
};

constexpr FaceFrame kWronged[] = {
    {FaceExpression::Awake, 120},
    {FaceExpression::Wronged, 760},
};

constexpr FaceFrame kSquint[] = {
    {FaceExpression::Awake, 160},
    {FaceExpression::Squint, 1200},
};

constexpr FaceFrame kInnocent[] = {
    {FaceExpression::Squint, 120},
    {FaceExpression::Innocent, 820},
};

constexpr FaceFrame kConfused[] = {
    {FaceExpression::Awake, 120},
    {FaceExpression::Confused, 360},
    {FaceExpression::Wronged, 260},
    {FaceExpression::Innocent, 420},
};

template <size_t N>
const FaceFrame* expose(const FaceFrame (&items)[N], size_t& count) {
  count = N;
  return items;
}

}  // namespace

const FaceFrame* FacePack::frames(FaceAction action, size_t& count) const {
  switch (action) {
    case FaceAction::WakeUp:
      return expose(kWakeUp, count);
    case FaceAction::Smile:
      return expose(kSmile, count);
    case FaceAction::Serious:
      return expose(kSerious, count);
    case FaceAction::RollEyes:
      return expose(kRollEyes, count);
    case FaceAction::Sleepy:
      return expose(kSleepy, count);
    case FaceAction::Wronged:
      return expose(kWronged, count);
    case FaceAction::Squint:
      return expose(kSquint, count);
    case FaceAction::Innocent:
      return expose(kInnocent, count);
    case FaceAction::Confused:
      return expose(kConfused, count);
    case FaceAction::None:
    default:
      return expose(kNone, count);
  }
}

}  // namespace tongdou
