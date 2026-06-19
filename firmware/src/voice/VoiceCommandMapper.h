#pragma once

#include "scenario/ScenarioEvent.h"
#include "voice/VoiceCommand.h"

namespace tongdou {

class VoiceCommandMapper {
 public:
  ScenarioEvent map(const VoiceCommandFrame& frame) const;
};

}  // namespace tongdou
