#pragma once

#include <stdint.h>

#include "voice/CiVoiceProtocol.h"

namespace tongdou {

class VoiceCommandParser {
 public:
  bool feed(uint8_t byte, VoiceCommandFrame& frame);
  void reset();

 private:
  CiVoiceProtocol protocol_;
};

}  // namespace tongdou
