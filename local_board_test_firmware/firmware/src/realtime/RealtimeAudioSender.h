#pragma once

#include <Arduino.h>

namespace tongdou {

struct RealtimeAudioSendStats {
  bool ready = false;
  uint32_t chunksSent = 0;
  uint32_t samplesSent = 0;
  uint32_t bytesSent = 0;
};

class RealtimeAudioSender {
 public:
  void begin();
  void reset();
  bool sendPcm16(const int16_t* samples, size_t sampleCount);
  RealtimeAudioSendStats stats() const;

 private:
  RealtimeAudioSendStats stats_;
};

}  // namespace tongdou
