#include "realtime/RealtimeAudioSender.h"

namespace tongdou {

void RealtimeAudioSender::begin() {
  stats_ = {};
  stats_.ready = true;
}

void RealtimeAudioSender::reset() {
  const bool wasReady = stats_.ready;
  stats_ = {};
  stats_.ready = wasReady;
}

bool RealtimeAudioSender::sendPcm16(const int16_t* samples, size_t sampleCount) {
  if (!stats_.ready || samples == nullptr || sampleCount == 0) {
    return false;
  }

  ++stats_.chunksSent;
  stats_.samplesSent += static_cast<uint32_t>(sampleCount);
  stats_.bytesSent += static_cast<uint32_t>(sampleCount * sizeof(samples[0]));
  return true;
}

RealtimeAudioSendStats RealtimeAudioSender::stats() const {
  return stats_;
}

}  // namespace tongdou
