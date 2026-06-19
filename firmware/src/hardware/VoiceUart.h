#pragma once

#include "scenario/ScenarioEvent.h"
#include "voice/VoiceCommandMapper.h"
#include "voice/VoiceCommandParser.h"

namespace tongdou {

class VoiceUart {
 public:
  void begin();
  bool readEvent(ScenarioEvent& event);

 private:
  VoiceCommandParser parser_;
  VoiceCommandMapper mapper_;
};

}  // namespace tongdou
