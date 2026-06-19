#pragma once

#include "scenario/ScenarioEvent.h"
#include "voice/VoiceCommand.h"

namespace tongdou {

class VoiceCommandMapper {
 public:
  bool map(const VoiceCommandFrame& frame, ScenarioEvent& event) const;
};

}  // namespace tongdou
