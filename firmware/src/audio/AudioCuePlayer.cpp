#include "audio/AudioCuePlayer.h"

#include <cstring>

#include "audio/OggOpusDemuxer.h"

#include "audio/AudioCueResourceTable.inc"

#define DECLARE_AUDIO_CUE(path, symbol) \
  extern const uint8_t symbol##_start[] asm("_binary_" #symbol "_start"); \
  extern const uint8_t symbol##_end[] asm("_binary_" #symbol "_end");
TONGDOU_AUDIO_CUE_TABLE(DECLARE_AUDIO_CUE)
#undef DECLARE_AUDIO_CUE

namespace tongdou {
namespace {

constexpr uint8_t kMaxLocaleFallbacks = 2;

struct EmbeddedAudioCue {
  const char* path;
  const uint8_t* start;
  const uint8_t* end;
};

#define AUDIO_CUE_ITEM(path, symbol) {path, symbol##_start, symbol##_end},
const EmbeddedAudioCue kAudioCues[] = {
    TONGDOU_AUDIO_CUE_TABLE(AUDIO_CUE_ITEM)
};
#undef AUDIO_CUE_ITEM

}  // namespace

AudioCuePlayer::AudioCuePlayer(AudioOutput& audioOutput) : audioOutput_(audioOutput) {}

AudioCuePlayer::~AudioCuePlayer() {
  closeDecoder();
}

bool AudioCuePlayer::begin() {
  if (sizeof(kAudioCues) / sizeof(kAudioCues[0]) == 0) {
    lastError_ = "audio_cue_table_empty";
    ready_ = false;
    return false;
  }

  ready_ = true;
  lastError_ = "";
  return true;
}

bool AudioCuePlayer::ready() const {
  return ready_;
}

const String& AudioCuePlayer::lastError() const {
  return lastError_;
}

AudioCuePlaybackResult AudioCuePlayer::playSystem(SystemAudioCue cue) {
  return playPath(systemPath(cue));
}

AudioCuePlaybackResult AudioCuePlayer::playLocalized(LocalizedAudioCue cue,
                                                     const String& locale) {
  const String primaryLocale = normalizedLocale(locale);
  const char* fileName = localizedName(cue);
  const String paths[kMaxLocaleFallbacks] = {
      makeLocalizedPath(primaryLocale, fileName),
      makeLocalizedPath("en-US", fileName),
  };

  for (const String& path : paths) {
    CueFile file;
    if (findFile(path.c_str(), file)) {
      return playPath(path.c_str());
    }
  }

  return fail("audio_cue_not_found");
}

AudioCuePlaybackResult AudioCuePlayer::playDigit(char digit, const String& locale) {
  if (digit < '0' || digit > '9') {
    return fail("invalid_audio_cue_digit");
  }

  String fileName = "digit_";
  fileName += digit;
  fileName += ".ogg";
  const String primaryLocale = normalizedLocale(locale);
  const String paths[kMaxLocaleFallbacks] = {
      makeLocalizedPath(primaryLocale, fileName.c_str()),
      makeLocalizedPath("en-US", fileName.c_str()),
  };

  for (const String& path : paths) {
    CueFile file;
    if (findFile(path.c_str(), file)) {
      return playPath(path.c_str());
    }
  }

  return fail("audio_cue_not_found");
}

AudioCuePlaybackResult AudioCuePlayer::playActivationCode(const String& code,
                                                          const String& locale) {
  AudioCuePlaybackResult result = playLocalized(LocalizedAudioCue::ActivationCode, locale);
  if (!result.ok) {
    return result;
  }

  for (size_t i = 0; i < code.length(); ++i) {
    const char digit = code.charAt(i);
    if (digit < '0' || digit > '9') {
      continue;
    }
    const AudioCuePlaybackResult digitResult = playDigit(digit, locale);
    if (!digitResult.ok) {
      return digitResult;
    }
    result.packets += digitResult.packets;
    result.samples += digitResult.samples;
    result.sampleRate = digitResult.sampleRate;
    delay(80);
  }

  result.ok = true;
  lastError_ = "";
  return result;
}

AudioCuePlaybackResult AudioCuePlayer::playPath(const char* path) {
  if (!ready_) {
    return fail("audio_cue_player_not_ready");
  }
  if (!audioOutput_.ready()) {
    return fail("audio_output_not_ready");
  }

  CueFile file;
  if (!findFile(path, file)) {
    return fail("audio_cue_not_found");
  }

  closeDecoder();
  OggOpusDemuxer demuxer;
  AudioCuePlaybackResult result;
  bool decodeOk = true;
  demuxer.reset();
  if (!demuxer.process(file.data, file.size, [&](const OggOpusPacket& packet) {
        if (!ensureDecoder(packet.sampleRate)) {
          decodeOk = false;
          return false;
        }

        const int decodedSamples =
            opus_decode(decoder_, packet.data, static_cast<opus_int32>(packet.size),
                        decoded_, static_cast<int>(sizeof(decoded_) / sizeof(decoded_[0])), 0);
        if (decodedSamples <= 0) {
          lastError_ = "opus_decode_failed";
          decodeOk = false;
          return false;
        }

        if (!audioOutput_.writePcm16Mono(decoded_, static_cast<size_t>(decodedSamples),
                                         packet.sampleRate)) {
          lastError_ = "audio_output_write_failed";
          decodeOk = false;
          return false;
        }

        ++result.packets;
        result.samples += static_cast<uint32_t>(decodedSamples);
        result.sampleRate = packet.sampleRate;
        return true;
      })) {
    return fail(lastError_.length() > 0 ? lastError_ : demuxer.error());
  }
  if (!decodeOk) {
    return fail(lastError_);
  }
  if (!demuxer.finish()) {
    return fail(demuxer.error()[0] == '\0' ? "ogg_opus_incomplete" : demuxer.error());
  }

  result.ok = true;
  lastError_ = "";
  return result;
}

const char* AudioCuePlayer::systemPath(SystemAudioCue cue) {
  switch (cue) {
    case SystemAudioCue::Wake:
      return "system/wake.ogg";
    case SystemAudioCue::Success:
      return "system/success.ogg";
    case SystemAudioCue::Error:
      return "system/error.ogg";
    case SystemAudioCue::LowBattery:
      return "system/low_battery.ogg";
    case SystemAudioCue::Vibration:
    default:
      return "system/vibration.ogg";
  }
}

const char* AudioCuePlayer::localizedName(LocalizedAudioCue cue) {
  switch (cue) {
    case LocalizedAudioCue::Welcome:
      return "welcome.ogg";
    case LocalizedAudioCue::WifiConfig:
      return "wifi_config.ogg";
    case LocalizedAudioCue::Upgrade:
      return "upgrade.ogg";
    case LocalizedAudioCue::ActivationCode:
      return "activation_code.ogg";
    case LocalizedAudioCue::ActivationError:
      return "activation_error.ogg";
    case LocalizedAudioCue::PinError:
    default:
      return "pin_error.ogg";
  }
}

bool AudioCuePlayer::findFile(const char* path, CueFile& file) const {
  if (path == nullptr) {
    return false;
  }

  for (const EmbeddedAudioCue& cue : kAudioCues) {
    if (strcmp(cue.path, path) == 0) {
      file.data = cue.start;
      file.size = static_cast<size_t>(cue.end - cue.start);
      return true;
    }
  }

  return false;
}

bool AudioCuePlayer::ensureDecoder(uint32_t sampleRate) {
  if (decoder_ != nullptr && decoderSampleRate_ == sampleRate) {
    return true;
  }

  closeDecoder();
  int error = OPUS_OK;
  decoder_ = opus_decoder_create(static_cast<opus_int32>(sampleRate), 1, &error);
  if (error != OPUS_OK || decoder_ == nullptr) {
    lastError_ = "opus_decoder_create_failed";
    closeDecoder();
    return false;
  }

  decoderSampleRate_ = sampleRate;
  return true;
}

void AudioCuePlayer::closeDecoder() {
  if (decoder_ != nullptr) {
    opus_decoder_destroy(decoder_);
    decoder_ = nullptr;
  }
  decoderSampleRate_ = 0;
}

String AudioCuePlayer::makeLocalizedPath(const String& locale, const char* fileName) const {
  String path = "locales/";
  path += locale;
  path += "/";
  path += localeFilePrefix(locale);
  path += "_";
  path += fileName;
  return path;
}

String AudioCuePlayer::normalizedLocale(const String& locale) const {
  String value = locale;
  value.trim();
  value.toLowerCase();
  if (value.length() == 0) {
    return "en-GB";
  }
  if (value == "en-gb") {
    return "en-GB";
  }
  if (value == "en-ca") {
    return "en-CA";
  }
  if (value == "fr-fr") {
    return "fr-FR";
  }
  if (value == "fr-ca") {
    return "fr-CA";
  }
  if (value == "nl-nl") {
    return "nl-NL";
  }
  if (value == "ru-ru") {
    return "ru-RU";
  }
  if (value == "zh-cn") {
    return "zh-CN";
  }
  return "en-GB";
}

String AudioCuePlayer::localeFilePrefix(const String& locale) const {
  String prefix = locale;
  prefix.replace("-", "_");
  return prefix;
}

AudioCuePlaybackResult AudioCuePlayer::fail(const String& error) {
  AudioCuePlaybackResult result;
  result.error = error;
  lastError_ = error;
  return result;
}

}  // namespace tongdou
