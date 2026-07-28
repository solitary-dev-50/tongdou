#pragma once

#include <stdint.h>

namespace tongdou {

enum class DemoSceneId : uint8_t {
  IdleStop = 0,
  LateNightDoghouseHook = 1,
  LateNightCowardTail = 2,
  MorningWakeup = 3,
  EmailSilentNotice = 4,
  RealtimeEmailSummary = 5,
  ConfusedAccountantGlobal = 6,
  CrowdfundingCallGlobal = 7,
  FirstSummonNightmare = 8,
};

const char* demoSceneSlug(DemoSceneId id);
const char* demoSceneSlug(DemoSceneId id, uint8_t variant);
const char* demoSceneTitle(DemoSceneId id);
const char* demoSceneTitle(DemoSceneId id, uint8_t variant);
DemoSceneId demoSceneFromNumber(uint8_t number);

}  // namespace tongdou
