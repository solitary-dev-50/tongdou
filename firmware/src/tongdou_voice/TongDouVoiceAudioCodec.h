#pragma once

#include <Arduino.h>

#include "tongdou_voice/TongDouVoiceConfig.h"

struct OpusDecoder;
struct OpusEncoder;

namespace tongdou {

struct TongDouVoiceAudioCodecStats {
  bool ready = false;
  uint32_t encodeAttempts = 0;
  uint32_t decodeAttempts = 0;
  uint32_t encodeFailures = 0;
  uint32_t decodeFailures = 0;
  String lastError;
};

class TongDouVoiceAudioCodec {
 public:
  ~TongDouVoiceAudioCodec();

  bool begin(const TongDouVoiceAudioParams& audio);
  void end();
  bool encodePcm16ToOpus(const int16_t* pcm, size_t sampleCount, uint8_t* opus,
                         size_t opusCapacity, size_t& opusLength);
  bool decodeOpusToPcm16(const uint8_t* opus, size_t opusLength, int16_t* pcm,
                         size_t pcmCapacity, size_t& sampleCount);
  bool ready() const;
  TongDouVoiceAudioCodecStats stats() const;

 private:
  void failEncode(const char* code);
  void failDecode(const char* code);

  TongDouVoiceAudioCodecStats stats_;
  TongDouVoiceAudioParams audio_;
  OpusEncoder* encoder_ = nullptr;
  OpusDecoder* decoder_ = nullptr;
  size_t frameSamples_ = 0;
};

}  // namespace tongdou
