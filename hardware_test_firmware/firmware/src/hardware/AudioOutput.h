#pragma once

#include <Arduino.h>

namespace tongdou {

class AudioOutput {
 public:
  void begin();
  void playTestTone(uint16_t frequencyHz = 880, uint16_t durationMs = 120);
  bool ready() const;
  uint8_t volumePercent() const;
  void setVolumePercent(uint8_t percent);

 private:
  bool ready_ = false;
  uint32_t currentSampleRate_ = 0;
  uint8_t volumePercent_ = 80;
};

}  // namespace tongdou
