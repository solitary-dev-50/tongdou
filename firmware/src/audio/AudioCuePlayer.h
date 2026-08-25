#pragma once

#include <Arduino.h>

#include <opus.h>

#include "hardware/AudioOutput.h"

namespace tongdou {

class LocaleResourceStore;

enum class SystemAudioCue : uint8_t {
  Wake,
  Success,
  Error,
  LowBattery,
  Vibration,
};

enum class LocalizedAudioCue : uint8_t {
  Welcome,
  WifiConfig,
  Upgrade,
  ActivationCode,
  ActivationError,
  PinError,
  OfflineNotice,
};

struct AudioCuePlaybackResult {
  bool ok = false;
  uint16_t packets = 0;
  uint32_t samples = 0;
  uint32_t sampleRate = 0;
  String error;
};

struct OfflineNoticePlaybackResult {
  AudioCuePlaybackResult speech;
  bool speechDrained = false;
  uint8_t tonesPlayed = 0;
};

class AudioCuePlayer {
 public:
  explicit AudioCuePlayer(AudioOutput& audioOutput);
  ~AudioCuePlayer();

  bool begin();
  void useLocaleResourceStore(LocaleResourceStore& store);
  bool ready() const;
  const String& lastError() const;

  AudioCuePlaybackResult playSystem(SystemAudioCue cue);
  AudioCuePlaybackResult playLocalized(LocalizedAudioCue cue, const String& locale);
  AudioCuePlaybackResult playDigit(char digit, const String& locale);
  AudioCuePlaybackResult playActivationCode(const String& code, const String& locale);
  OfflineNoticePlaybackResult playOfflineNotice(const String& locale);
  AudioCuePlaybackResult playPath(const char* path);

 private:
  struct CueFile {
    const uint8_t* data = nullptr;
    size_t size = 0;
  };

  static const char* systemPath(SystemAudioCue cue);
  static const char* localizedName(LocalizedAudioCue cue);
  bool findFile(const char* path, CueFile& file) const;
  AudioCuePlaybackResult playLocaleFile(const char* fileName);
  AudioCuePlaybackResult playBuffer(const uint8_t* data, size_t size);
  bool ensureDecoder(uint32_t sampleRate);
  void closeDecoder();
  String makeLocalizedPath(const String& locale, const char* fileName) const;
  String normalizedLocale(const String& locale) const;
  String localeFilePrefix(const String& locale) const;
  AudioCuePlaybackResult fail(const String& error);

  AudioOutput& audioOutput_;
  LocaleResourceStore* localeResources_ = nullptr;
  OpusDecoder* decoder_ = nullptr;
  uint32_t decoderSampleRate_ = 0;
  bool ready_ = false;
  String lastError_;
  int16_t decoded_[5760] = {};
};

}  // namespace tongdou
