#pragma once

#include "scenario/ScenarioEvent.h"
#include "voice/CiVoiceProtocol.h"
#include "voice/VoiceCommandMapper.h"
#include "voice/VoiceCommandParser.h"

namespace tongdou {

class VoiceUart {
 public:
  void begin();
  bool readEvent(ScenarioEvent& event);
  void playVoice(uint16_t voiceId);

 private:
  uint8_t nextSequence();

  VoiceCommandParser parser_;
  VoiceCommandMapper mapper_;
  CiVoiceProtocol protocol_;
  uint8_t sequence_ = 0;
};

}  // namespace tongdou
