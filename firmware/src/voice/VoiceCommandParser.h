#pragma once

#include <stdint.h>

#include "voice/VoiceCommand.h"

namespace tongdou {

class VoiceCommandParser {
 public:
  bool feed(uint8_t byte, VoiceCommandFrame& frame);
  void reset();

 private:
  bool waitingForCommand_ = false;
};

}  // namespace tongdou
