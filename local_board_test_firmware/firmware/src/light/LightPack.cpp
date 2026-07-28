#include "light/LightPack.h"

namespace tongdou {
namespace {

constexpr LightFrame kOff[] = {{{0, 0, 0}, 0}};
constexpr LightFrame kSoftWhite[] = {{{28, 26, 20}, 320}};
constexpr LightFrame kWarmWake[] = {
    {{0, 0, 0}, 80},
    {{8, 4, 1}, 120},
    {{18, 10, 3}, 160},
    {{32, 18, 5}, 420},
};
constexpr LightFrame kRedShortBlink[] = {
    {{42, 0, 0}, 120},
    {{0, 0, 0}, 80},
    {{42, 0, 0}, 140},
    {{0, 0, 0}, 0},
};
constexpr LightFrame kWeakBreath[] = {
    {{4, 4, 4}, 260},
    {{10, 9, 7}, 420},
    {{3, 3, 3}, 520},
};
constexpr LightFrame kDimWarm[] = {{{12, 6, 2}, 600}};
constexpr LightFrame kDimCyan[] = {
    {{0, 8, 10}, 260},
    {{0, 16, 18}, 420},
    {{0, 6, 8}, 520},
};
constexpr LightFrame kSummonCyanWake[] = {
    {{0, 0, 0}, 80},
    {{0, 10, 18}, 360},
    {{0, 18, 28}, 520},
};
constexpr LightFrame kSummonCyanHold[] = {{{0, 16, 24}, 0}};
constexpr LightFrame kSummonCyanFocus[] = {{{0, 28, 42}, 0}};
constexpr LightFrame kSummonPurpleApproach[] = {
    {{0, 24, 34}, 360},
    {{8, 8, 34}, 620},
    {{14, 4, 38}, 0},
};
constexpr LightFrame kSummonGoodFlash[] = {
    {{48, 42, 64}, 100},
    {{14, 4, 38}, 0},
};
constexpr LightFrame kSummonPurpleHold[] = {{{12, 4, 34}, 0}};
constexpr LightFrame kSummonNightmareRise[] = {
    {{10, 4, 32}, 520},
    {{18, 8, 52}, 620},
    {{26, 12, 72}, 0},
};
constexpr LightFrame kSummonDrop[] = {{{0, 8, 16}, 0}};
constexpr LightFrame kSummonNowFlash[] = {
    {{70, 90, 96}, 130},
    {{0, 18, 28}, 0},
};
constexpr LightFrame kSummonLaughBreath[] = {
    {{0, 18, 28}, 140},
    {{0, 34, 46}, 260},
    {{0, 14, 22}, 220},
    {{0, 34, 46}, 260},
    {{0, 12, 20}, 0},
};
constexpr LightFrame kSummonIdleCyan[] = {{{0, 10, 16}, 0}};

template <size_t Count>
LightSequence makeSequence(const LightFrame (&frames)[Count]) {
  return {frames, Count};
}

}  // namespace

LightSequence LightPack::sequence(LightAction action) const {
  switch (action) {
    case LightAction::Off:
      return makeSequence(kOff);
    case LightAction::SoftWhite:
      return makeSequence(kSoftWhite);
    case LightAction::WarmWake:
      return makeSequence(kWarmWake);
    case LightAction::RedShortBlink:
      return makeSequence(kRedShortBlink);
    case LightAction::WeakBreath:
      return makeSequence(kWeakBreath);
    case LightAction::DimWarm:
      return makeSequence(kDimWarm);
    case LightAction::DimCyan:
      return makeSequence(kDimCyan);
    case LightAction::SummonCyanWake:
      return makeSequence(kSummonCyanWake);
    case LightAction::SummonCyanHold:
      return makeSequence(kSummonCyanHold);
    case LightAction::SummonCyanFocus:
      return makeSequence(kSummonCyanFocus);
    case LightAction::SummonPurpleApproach:
      return makeSequence(kSummonPurpleApproach);
    case LightAction::SummonGoodFlash:
      return makeSequence(kSummonGoodFlash);
    case LightAction::SummonPurpleHold:
      return makeSequence(kSummonPurpleHold);
    case LightAction::SummonNightmareRise:
      return makeSequence(kSummonNightmareRise);
    case LightAction::SummonDrop:
      return makeSequence(kSummonDrop);
    case LightAction::SummonNowFlash:
      return makeSequence(kSummonNowFlash);
    case LightAction::SummonLaughBreath:
      return makeSequence(kSummonLaughBreath);
    case LightAction::SummonIdleCyan:
      return makeSequence(kSummonIdleCyan);
    case LightAction::None:
    default:
      return {};
  }
}

}  // namespace tongdou
