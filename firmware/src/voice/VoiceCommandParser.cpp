#include "voice/VoiceCommandParser.h"

namespace tongdou {

bool VoiceCommandParser::feed(uint8_t byte, VoiceCommandFrame& frame) {
  return protocol_.feed(byte, frame);
}

void VoiceCommandParser::reset() {
  protocol_.reset();
}

}  // namespace tongdou
