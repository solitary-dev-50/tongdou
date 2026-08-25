#pragma once

#include <Arduino.h>

namespace tongdou {

class AudioOutput {
 public:
  void begin();
  bool playTone(uint16_t frequencyHz, uint16_t durationMs);
  void playTestTone(uint16_t frequencyHz = 880, uint16_t durationMs = 120);
  void playClip(uint16_t clipId, const char* text);
  bool writePcm16Mono(const int16_t* samples, size_t sampleCount, uint32_t sampleRate);
  void stopStream();
  bool ready() const;
  bool streamDrained() const;

 private:
  bool ready_ = false;
  uint32_t currentSampleRate_ = 0;
  unsigned long streamDrainAtMs_ = 0;
  bool streamActive_ = false;
};

}  // namespace tongdou
