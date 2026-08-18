#pragma once

#include <Arduino.h>

namespace tongdou {

struct TongDouVoiceQueueStats {
  uint16_t downlinkQueued = 0;
  uint32_t downlinkPushed = 0;
  uint32_t downlinkPopped = 0;
  uint32_t downlinkDropped = 0;
  uint32_t downlinkOversized = 0;
};

class TongDouVoiceStreamQueues {
 public:
  static constexpr size_t kDownlinkCapacity = 12;
  static constexpr size_t kMaxPacketBytes = 512;

  void clear();
  bool pushDownlink(const uint8_t* data, size_t length);
  bool popDownlink(uint8_t* output, size_t maxLength, size_t& length);
  TongDouVoiceQueueStats stats() const;

 private:
  struct Packet {
    uint16_t length = 0;
    uint8_t data[kMaxPacketBytes] = {};
  };

  void dropOldest();

  Packet downlink_[kDownlinkCapacity];
  size_t readIndex_ = 0;
  size_t writeIndex_ = 0;
  size_t count_ = 0;
  TongDouVoiceQueueStats stats_;
};

}  // namespace tongdou
