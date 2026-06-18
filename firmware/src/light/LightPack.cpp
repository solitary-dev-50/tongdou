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
    case LightAction::None:
    default:
      return {};
  }
}

}  // namespace tongdou
