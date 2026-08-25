#include "audio/AudioVolumeService.h"

#include <Preferences.h>

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_audio";
constexpr const char* kVolumeKey = "volume";
constexpr uint8_t kDefaultVolumePercent = 100;
constexpr uint8_t kMaximumVolumePercent = 100;

Preferences preferences;

}  // namespace

AudioVolumeService::AudioVolumeService(AudioOutput& audioOutput)
    : audioOutput_(audioOutput) {}

bool AudioVolumeService::begin() {
  volumePercent_ = kDefaultVolumePercent;
  ready_ = preferences.begin(kNamespace, false);
  if (ready_ && preferences.isKey(kVolumeKey)) {
    const uint8_t saved = preferences.getUChar(
        kVolumeKey, kDefaultVolumePercent);
    if (saved <= kMaximumVolumePercent) {
      volumePercent_ = saved;
    } else {
      preferences.putUChar(kVolumeKey, kDefaultVolumePercent);
    }
  } else if (ready_) {
    preferences.putUChar(kVolumeKey, kDefaultVolumePercent);
  }

  audioOutput_.setVolume(volumePercent_);
  return ready_;
}

bool AudioVolumeService::setVolume(uint8_t percent) {
  if (!ready_ || percent > kMaximumVolumePercent) {
    return false;
  }
  if (percent == volumePercent_) {
    return true;
  }
  if (preferences.putUChar(kVolumeKey, percent) == 0) {
    return false;
  }

  volumePercent_ = percent;
  audioOutput_.setVolume(volumePercent_);
  return true;
}

uint8_t AudioVolumeService::volume() const {
  return volumePercent_;
}

bool AudioVolumeService::ready() const {
  return ready_;
}

}  // namespace tongdou
