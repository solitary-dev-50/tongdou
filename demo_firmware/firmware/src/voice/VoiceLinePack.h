#pragma once

#include <stdint.h>

#include "scenario/ScenarioAction.h"

namespace tongdou {

struct VoiceClip {
  uint16_t clipId = 0;
  const char* fileName = "";
  const char* text = "";
};

class VoiceLinePack {
 public:
  VoiceClip clip(VoiceLine line) const;
};

}  // namespace tongdou
