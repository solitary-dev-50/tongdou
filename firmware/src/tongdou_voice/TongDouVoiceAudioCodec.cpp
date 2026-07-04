#include "tongdou_voice/TongDouVoiceAudioCodec.h"

#include <opus.h>

namespace tongdou {
namespace {

constexpr int kOpusBitrate = 24000;
constexpr int kOpusComplexity = 3;
constexpr size_t kSelfTestMaxSamples = 960;
constexpr size_t kSelfTestOpusCapacity = 512;
constexpr int16_t kSelfTestAmplitude = 9000;

bool isSupportedSampleRate(uint16_t sampleRate) {
  return sampleRate == 8000 || sampleRate == 12000 || sampleRate == 16000 ||
         sampleRate == 24000 || sampleRate == 48000;
}

uint32_t meanAbs(const int16_t* samples, size_t sampleCount) {
  if (samples == nullptr || sampleCount == 0) {
    return 0;
  }

  uint64_t sum = 0;
  for (size_t i = 0; i < sampleCount; ++i) {
    const int32_t sample = samples[i];
    sum += static_cast<uint32_t>(sample < 0 ? -sample : sample);
  }
  return static_cast<uint32_t>(sum / sampleCount);
}

}  // namespace

TongDouVoiceAudioCodec::~TongDouVoiceAudioCodec() {
  end();
}

bool TongDouVoiceAudioCodec::begin(const TongDouVoiceAudioParams& audio) {
  end();
  audio_ = audio;
  stats_ = {};

  if (audio_.format != "opus") {
    stats_.lastError = "unsupported_audio_format";
    return false;
  }
  if (audio_.channels != 1) {
    stats_.lastError = "unsupported_audio_channels";
    return false;
  }
  if (!isSupportedSampleRate(audio_.sampleRate)) {
    stats_.lastError = "unsupported_audio_sample_rate";
    return false;
  }
  if (audio_.frameDurationMs == 0) {
    stats_.lastError = "invalid_audio_frame_duration";
    return false;
  }

  frameSamples_ =
      (static_cast<size_t>(audio_.sampleRate) * audio_.frameDurationMs) / 1000U;
  if (frameSamples_ == 0) {
    stats_.lastError = "invalid_audio_frame_samples";
    return false;
  }

  int error = OPUS_OK;
  encoder_ = opus_encoder_create(audio_.sampleRate, audio_.channels,
                                 OPUS_APPLICATION_VOIP, &error);
  if (error != OPUS_OK || encoder_ == nullptr) {
    stats_.lastError = "opus_encoder_create_failed";
    end();
    return false;
  }

  decoder_ = opus_decoder_create(audio_.sampleRate, audio_.channels, &error);
  if (error != OPUS_OK || decoder_ == nullptr) {
    stats_.lastError = "opus_decoder_create_failed";
    end();
    return false;
  }

  opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(kOpusBitrate));
  opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(kOpusComplexity));

  stats_.ready = true;
  stats_.lastError = "";
  return true;
}

void TongDouVoiceAudioCodec::end() {
  if (encoder_ != nullptr) {
    opus_encoder_destroy(encoder_);
    encoder_ = nullptr;
  }
  if (decoder_ != nullptr) {
    opus_decoder_destroy(decoder_);
    decoder_ = nullptr;
  }
  frameSamples_ = 0;
  stats_.ready = false;
}

TongDouVoiceAudioCodecSelfTestResult TongDouVoiceAudioCodec::runSelfTest() {
  TongDouVoiceAudioCodecSelfTestResult result;
  result.inputSamples = frameSamples_;
  if (!ready() || frameSamples_ == 0) {
    result.error = "opus_codec_not_ready";
    return result;
  }
  if (frameSamples_ > kSelfTestMaxSamples) {
    result.error = "self_test_frame_too_large";
    return result;
  }

  int16_t pcm[kSelfTestMaxSamples] = {};
  int16_t decoded[kSelfTestMaxSamples] = {};
  uint8_t opus[kSelfTestOpusCapacity] = {};

  for (size_t i = 0; i < frameSamples_; ++i) {
    const int16_t sample = (i % 64U) < 32U ? kSelfTestAmplitude : -kSelfTestAmplitude;
    pcm[i] = sample;
  }

  size_t opusLength = 0;
  if (!encodePcm16ToOpus(pcm, frameSamples_, opus, kSelfTestOpusCapacity,
                         opusLength)) {
    result.error = stats_.lastError;
  } else {
    size_t decodedSamples = 0;
    if (!decodeOpusToPcm16(opus, opusLength, decoded, frameSamples_, decodedSamples)) {
      result.error = stats_.lastError;
    } else {
      result.ok = true;
      result.encodedBytes = opusLength;
      result.decodedSamples = decodedSamples;
      result.inputMeanAbs = meanAbs(pcm, frameSamples_);
      result.decodedMeanAbs = meanAbs(decoded, decodedSamples);
    }
  }

  return result;
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
  if (!ready() || encoder_ == nullptr) {
    failEncode("opus_encoder_not_ready");
    return false;
  }
  if (sampleCount != frameSamples_) {
    failEncode("invalid_encode_frame_samples");
    return false;
  }

  const int encoded = opus_encode(encoder_, pcm, static_cast<int>(sampleCount),
                                  opus, static_cast<opus_int32>(opusCapacity));
  if (encoded <= 0) {
    failEncode("opus_encode_failed");
    return false;
  }

  opusLength = static_cast<size_t>(encoded);
  return true;
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
  if (!ready() || decoder_ == nullptr) {
    failDecode("opus_decoder_not_ready");
    return false;
  }

  const int decoded = opus_decode(decoder_, opus, static_cast<opus_int32>(opusLength),
                                  pcm, static_cast<int>(pcmCapacity), 0);
  if (decoded <= 0) {
    failDecode("opus_decode_failed");
    return false;
  }

  sampleCount = static_cast<size_t>(decoded);
  return true;
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
