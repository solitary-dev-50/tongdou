#include "tongdou_voice/TongDouVoiceStreamQueues.h"

#include <cstring>

namespace tongdou {

void TongDouVoiceStreamQueues::clear() {
  readIndex_ = 0;
  writeIndex_ = 0;
  count_ = 0;
  stats_.downlinkQueued = 0;
}

bool TongDouVoiceStreamQueues::pushDownlink(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    return false;
  }
  if (length > kMaxPacketBytes) {
    ++stats_.downlinkOversized;
    return false;
  }
  if (count_ >= kDownlinkCapacity) {
    dropOldest();
  }

  Packet& packet = downlink_[writeIndex_];
  packet.length = static_cast<uint16_t>(length);
  std::memcpy(packet.data, data, length);
  writeIndex_ = (writeIndex_ + 1) % kDownlinkCapacity;
  ++count_;
  ++stats_.downlinkPushed;
  stats_.downlinkQueued = static_cast<uint16_t>(count_);
  return true;
}

bool TongDouVoiceStreamQueues::popDownlink(uint8_t* output, size_t maxLength,
                                           size_t& length) {
  length = 0;
  if (output == nullptr || count_ == 0) {
    return false;
  }

  const Packet& packet = downlink_[readIndex_];
  if (packet.length > maxLength) {
    return false;
  }

  length = packet.length;
  std::memcpy(output, packet.data, length);
  readIndex_ = (readIndex_ + 1) % kDownlinkCapacity;
  --count_;
  ++stats_.downlinkPopped;
  stats_.downlinkQueued = static_cast<uint16_t>(count_);
  return true;
}

TongDouVoiceQueueStats TongDouVoiceStreamQueues::stats() const {
  TongDouVoiceQueueStats copy = stats_;
  copy.downlinkQueued = static_cast<uint16_t>(count_);
  return copy;
}

void TongDouVoiceStreamQueues::dropOldest() {
  if (count_ == 0) {
    return;
  }

  readIndex_ = (readIndex_ + 1) % kDownlinkCapacity;
  --count_;
  ++stats_.downlinkDropped;
  stats_.downlinkQueued = static_cast<uint16_t>(count_);
}

}  // namespace tongdou
