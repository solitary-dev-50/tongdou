#include "voice/CiVoiceProtocol.h"

namespace tongdou {
namespace {

void writeLe16(uint8_t* bytes, uint16_t value) {
  bytes[0] = static_cast<uint8_t>(value & 0xFF);
  bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void writeLe32(uint8_t* bytes, uint32_t value) {
  bytes[0] = static_cast<uint8_t>(value & 0xFF);
  bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint32_t readLe32(const uint8_t* bytes) {
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}

}  // namespace

bool CiVoiceProtocol::feed(uint8_t byte, VoiceCommandFrame& frame) {
  switch (state_) {
    case ParseState::WaitingForHeader:
      if (byte == kVoiceCommandHeader) {
        state_ = ParseState::LegacyCommand;
      } else if (byte == kCiProtocolHeadLow) {
        state_ = ParseState::HeaderHigh;
      }
      return false;

    case ParseState::LegacyCommand:
      frame.command = byte;
      reset();
      return true;

    case ParseState::HeaderHigh:
      if (byte == kCiProtocolHeadHigh) {
        state_ = ParseState::LengthLow;
      } else {
        reset();
      }
      return false;

    case ParseState::LengthLow:
      dataLength_ = byte;
      state_ = ParseState::LengthHigh;
      return false;

    case ParseState::LengthHigh:
      dataLength_ |= static_cast<uint16_t>(byte) << 8;
      if (dataLength_ > kCiMaxDataLength) {
        reset();
      } else {
        state_ = ParseState::Type;
      }
      return false;

    case ParseState::Type:
      type_ = byte;
      addChecksum(byte);
      state_ = ParseState::Command;
      return false;

    case ParseState::Command:
      command_ = byte;
      addChecksum(byte);
      state_ = ParseState::Sequence;
      return false;

    case ParseState::Sequence:
      sequence_ = byte;
      addChecksum(byte);
      dataIndex_ = 0;
      state_ = dataLength_ == 0 ? ParseState::ChecksumLow : ParseState::Data;
      return false;

    case ParseState::Data:
      data_[dataIndex_++] = byte;
      addChecksum(byte);
      if (dataIndex_ >= dataLength_) {
        state_ = ParseState::ChecksumLow;
      }
      return false;

    case ParseState::ChecksumLow:
      receivedChecksum_ = byte;
      state_ = ParseState::ChecksumHigh;
      return false;

    case ParseState::ChecksumHigh:
      receivedChecksum_ |= static_cast<uint16_t>(byte) << 8;
      state_ = ParseState::Tail;
      return false;

    case ParseState::Tail:
      if (byte != kCiProtocolTail) {
        reset();
        return false;
      }

      return finishFrame(frame);
  }

  reset();
  return false;
}

void CiVoiceProtocol::reset() {
  state_ = ParseState::WaitingForHeader;
  resetCiFrame();
}

VoicePlaybackFrame CiVoiceProtocol::makePlayVoiceFrame(uint16_t voiceId, uint8_t sequence) {
  VoicePlaybackFrame frame;
  frame.length = VoicePlaybackFrame::kMaxLength;

  frame.bytes[0] = kCiProtocolHeadLow;
  frame.bytes[1] = kCiProtocolHeadHigh;
  writeLe16(&frame.bytes[2], kCiPlayVoiceLength);
  frame.bytes[4] = kCiProtocolTypeCommandDown;
  frame.bytes[5] = kCiProtocolCommandPlayVoice;
  frame.bytes[6] = sequence;
  frame.bytes[7] = kCiProtocolPlayStart;
  frame.bytes[8] = kCiProtocolPlayByVoiceId;
  writeLe32(&frame.bytes[9], voiceId);

  uint16_t checksum = 0;
  for (uint8_t i = 4; i <= 12; ++i) {
    checksum = static_cast<uint16_t>(checksum + frame.bytes[i]);
  }

  writeLe16(&frame.bytes[13], checksum);
  frame.bytes[15] = kCiProtocolTail;
  return frame;
}

bool CiVoiceProtocol::finishFrame(VoiceCommandFrame& frame) {
  const bool checksumOk = checksum_ == receivedChecksum_;
  const bool asrResult = type_ == kCiProtocolTypeCommandUp &&
                         command_ == kCiProtocolCommandAsrResult &&
                         dataLength_ >= 4;

  if (checksumOk && asrResult) {
    frame.command = readLe32(data_);
    reset();
    return true;
  }

  reset();
  return false;
}

void CiVoiceProtocol::resetCiFrame() {
  dataLength_ = 0;
  dataIndex_ = 0;
  type_ = 0;
  command_ = 0;
  sequence_ = 0;
  checksum_ = 0;
  receivedChecksum_ = 0;
}

void CiVoiceProtocol::addChecksum(uint8_t byte) {
  checksum_ = static_cast<uint16_t>(checksum_ + byte);
}

}  // namespace tongdou
