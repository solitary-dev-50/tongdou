#include "tongdou_voice/TongDouVoiceAudioCodec.h"

namespace tongdou {

bool TongDouVoiceAudioCodec::begin(const TongDouVoiceAudioParams& audio) {
  audio_ = audio;
  stats_ = {};

  if (audio_.format != "opus") {
    stats_.lastError = "unsupported_audio_format";
    return false;
  }

  stats_.lastError = "opus_codec_not_linked";
  return false;
}

bool TongDouVoiceAudioCodec::encodePcm16ToOpus(const int16_t* pcm,
                                               size_t sampleCount, uint8_t* opus,
                                               size_t opusCapacity,
                                               size_t& opusLength) {
  opusLength = 0;
  ++stats_.encodeAttempts;
  if (pcm == nullptr || sampleCount == 0 || opus == nullptr || opusCapacity == 0) {
    failEncode("invalid_encode_buffer");
    return false;
  }

  failEncode("opus_encoder_not_ready");
  return false;
}

bool TongDouVoiceAudioCodec::decodeOpusToPcm16(const uint8_t* opus,
                                               size_t opusLength, int16_t* pcm,
                                               size_t pcmCapacity,
                                               size_t& sampleCount) {
  sampleCount = 0;
  ++stats_.decodeAttempts;
  if (opus == nullptr || opusLength == 0 || pcm == nullptr || pcmCapacity == 0) {
    failDecode("invalid_decode_buffer");
    return false;
  }

  failDecode("opus_decoder_not_ready");
  return false;
}

bool TongDouVoiceAudioCodec::ready() const {
  return stats_.ready;
}

TongDouVoiceAudioCodecStats TongDouVoiceAudioCodec::stats() const {
  return stats_;
}

void TongDouVoiceAudioCodec::failEncode(const char* code) {
  ++stats_.encodeFailures;
  stats_.lastError = code;
}

void TongDouVoiceAudioCodec::failDecode(const char* code) {
  ++stats_.decodeFailures;
  stats_.lastError = code;
}

}  // namespace tongdou
