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
  CheekyDanceGlobal = 6,
  CrowdfundingCallGlobal = 7,
  FirstSummonNightmare = 8,
};

const char* demoSceneSlug(DemoSceneId id);
const char* demoSceneTitle(DemoSceneId id);
DemoSceneId demoSceneFromNumber(uint8_t number);

}  // namespace tongdou
