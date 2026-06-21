#pragma once

#include <Arduino.h>

namespace tongdou {

class AudioOutput {
 public:
  void begin();
  void playTestTone(uint16_t frequencyHz = 880, uint16_t durationMs = 120);
  void playClip(uint16_t clipId, const char* text);
  bool ready() const;

 private:
  bool ready_ = false;
};

}  // namespace tongdou
