#pragma once

#include <Arduino.h>

namespace tongdou {

class AudioOutput {
 public:
  void begin();
  void playClip(uint16_t clipId, const char* text);
};

}  // namespace tongdou
