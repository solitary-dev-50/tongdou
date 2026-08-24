#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tongdou {

class PcmAudioSink {
 public:
  virtual ~PcmAudioSink() = default;
  virtual void offerPcm16(const int16_t* samples, size_t sampleCount) = 0;
};

}  // namespace tongdou
