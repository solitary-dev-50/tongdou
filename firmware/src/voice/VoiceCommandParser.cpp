#include "voice/VoiceCommandParser.h"

namespace tongdou {

bool VoiceCommandParser::feed(uint8_t byte, VoiceCommandFrame& frame) {
  if (!waitingForCommand_) {
    waitingForCommand_ = byte == kVoiceCommandHeader;
    return false;
  }

  frame.command = byte;
  waitingForCommand_ = false;
  return true;
}

void VoiceCommandParser::reset() {
  waitingForCommand_ = false;
}

}  // namespace tongdou
