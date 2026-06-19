#pragma once

#include <stdint.h>

#include "voice/VoiceCommand.h"

namespace tongdou {

class CiVoiceProtocol {
 public:
  bool feed(uint8_t byte, VoiceCommandFrame& frame);
  void reset();
  VoicePlaybackFrame makePlayVoiceFrame(uint16_t voiceId, uint8_t sequence);

 private:
  enum class ParseState : uint8_t {
    WaitingForHeader,
    LegacyCommand,
    HeaderHigh,
    LengthLow,
    LengthHigh,
    Type,
    Command,
    Sequence,
    Data,
    ChecksumLow,
    ChecksumHigh,
    Tail,
  };

  bool finishFrame(VoiceCommandFrame& frame);
  void resetCiFrame();
  void addChecksum(uint8_t byte);

  ParseState state_ = ParseState::WaitingForHeader;
  uint16_t dataLength_ = 0;
  uint8_t dataIndex_ = 0;
  uint8_t type_ = 0;
  uint8_t command_ = 0;
  uint8_t sequence_ = 0;
  uint8_t data_[kCiMaxDataLength] = {};
  uint16_t checksum_ = 0;
  uint16_t receivedChecksum_ = 0;
};

}  // namespace tongdou
