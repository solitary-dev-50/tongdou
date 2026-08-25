#pragma once

#include <Arduino.h>

#include "hardware/AudioOutput.h"

namespace tongdou {

class AudioVolumeService {
 public:
  explicit AudioVolumeService(AudioOutput& audioOutput);

  bool begin();
  bool setVolume(uint8_t percent);
  uint8_t volume() const;
  bool ready() const;

 private:
  AudioOutput& audioOutput_;
  uint8_t volumePercent_ = 100;
  bool ready_ = false;
};

}  // namespace tongdou
