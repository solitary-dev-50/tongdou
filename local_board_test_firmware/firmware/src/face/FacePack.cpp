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
    {FaceExpression::Blink, 70},
    {FaceExpression::HalfOpen, 120},
    {FaceExpression::Awake, 380},
};

constexpr FaceFrame kSleep[] = {
    {FaceExpression::Sleep, 800},
};

constexpr FaceFrame kAwake[] = {
    {FaceExpression::Awake, 800},
};

constexpr FaceFrame kBlink[] = {
    {FaceExpression::Awake, 120},
    {FaceExpression::HalfOpen, 80},
    {FaceExpression::Blink, 70},
    {FaceExpression::HalfOpen, 80},
    {FaceExpression::Awake, 240},
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

constexpr FaceFrame kFirstSummonConfusedHold[] = {
    {FaceExpression::Confused, 1200},
};

constexpr FaceFrame kAngry[] = {
    {FaceExpression::Awake, 100},
    {FaceExpression::Angry, 760},
};

constexpr FaceFrame kSurprised[] = {
    {FaceExpression::Awake, 80},
    {FaceExpression::Surprised, 520},
    {FaceExpression::Blink, 80},
    {FaceExpression::Awake, 240},
};

constexpr FaceFrame kShy[] = {
    {FaceExpression::Awake, 120},
    {FaceExpression::Shy, 760},
};

constexpr FaceFrame kFierce[] = {
    {FaceExpression::Squint, 100},
    {FaceExpression::Fierce, 820},
};

constexpr FaceFrame kProud[] = {
    {FaceExpression::Awake, 120},
    {FaceExpression::Proud, 760},
};

constexpr FaceFrame kNervous[] = {
    {FaceExpression::Squint, 120},
    {FaceExpression::Nervous, 420},
    {FaceExpression::Wronged, 360},
};

constexpr FaceFrame kAccountantPause[] = {
    {FaceExpression::AccountantPause, 900},
};

constexpr FaceFrame kAccountantRoundEyes[] = {
    {FaceExpression::AccountantRoundEyes, 700},
};

constexpr FaceFrame kAccountantSuspiciousSquint[] = {
    {FaceExpression::AccountantSuspiciousSquint, 1200},
};

constexpr FaceFrame kAccountantCalculating[] = {
    {FaceExpression::AccountantCalculating, 900},
};

constexpr FaceFrame kAccountantLedgerSearch[] = {
    {FaceExpression::AccountantLedgerSearchHigh, 180},
    {FaceExpression::AccountantLedgerSearchLow, 180},
    {FaceExpression::AccountantLedgerSearchHigh, 180},
    {FaceExpression::AccountantLedgerSearchLow, 180},
};

constexpr FaceFrame kAccountantDebt3[] = {
    {FaceExpression::AccountantDebt3, 900},
};

constexpr FaceFrame kAccountantDebtScratch[] = {
    {FaceExpression::AccountantDebtScratchStart, 180},
    {FaceExpression::AccountantDebtScratchMid, 220},
    {FaceExpression::AccountantDebtScratchEnd, 280},
};

constexpr FaceFrame kAccountantDebt2[] = {
    {FaceExpression::AccountantDebt2, 1000},
};

constexpr FaceFrame kAccountantDebtPi[] = {
    {FaceExpression::AccountantDebtPi, 1300},
};

constexpr FaceFrame kAccountantSmugSquint[] = {
    {FaceExpression::AccountantSmugSquint, 1200},
};

template <size_t N>
const FaceFrame* expose(const FaceFrame (&items)[N], size_t& count) {
  count = N;
  return items;
}

}  // namespace

const FaceFrame* FacePack::frames(FaceAction action, size_t& count) const {
  switch (action) {
    case FaceAction::Sleep:
      return expose(kSleep, count);
    case FaceAction::WakeUp:
      return expose(kWakeUp, count);
    case FaceAction::Awake:
      return expose(kAwake, count);
    case FaceAction::Blink:
      return expose(kBlink, count);
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
    case FaceAction::Angry:
      return expose(kAngry, count);
    case FaceAction::Surprised:
      return expose(kSurprised, count);
    case FaceAction::Shy:
      return expose(kShy, count);
    case FaceAction::Fierce:
      return expose(kFierce, count);
    case FaceAction::Proud:
      return expose(kProud, count);
    case FaceAction::Nervous:
      return expose(kNervous, count);
    case FaceAction::FirstSummonConfusedHold:
      return expose(kFirstSummonConfusedHold, count);
    case FaceAction::AccountantPause:
      return expose(kAccountantPause, count);
    case FaceAction::AccountantRoundEyes:
      return expose(kAccountantRoundEyes, count);
    case FaceAction::AccountantSuspiciousSquint:
      return expose(kAccountantSuspiciousSquint, count);
    case FaceAction::AccountantCalculating:
      return expose(kAccountantCalculating, count);
    case FaceAction::AccountantLedgerSearch:
      return expose(kAccountantLedgerSearch, count);
    case FaceAction::AccountantDebt3:
      return expose(kAccountantDebt3, count);
    case FaceAction::AccountantDebtScratch:
      return expose(kAccountantDebtScratch, count);
    case FaceAction::AccountantDebt2:
      return expose(kAccountantDebt2, count);
    case FaceAction::AccountantDebtPi:
      return expose(kAccountantDebtPi, count);
    case FaceAction::AccountantSmugSquint:
      return expose(kAccountantSmugSquint, count);
    case FaceAction::None:
    default:
      return expose(kNone, count);
  }
}

}  // namespace tongdou
