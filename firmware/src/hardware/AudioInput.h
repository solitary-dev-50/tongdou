#pragma once

#include <Arduino.h>

namespace tongdou {

struct AudioInputSnapshot {
  bool ready = false;
  bool hasSamples = false;
  size_t samplesRead = 0;
  int32_t peak = 0;
  int32_t averageAbs = 0;
};

class AudioInput {
 public:
  void begin();
  void update();
  AudioInputSnapshot readLevel();
  bool ready() const;

 private:
  bool ready_ = false;
  unsigned long lastLogMs_ = 0;
};

}  // namespace tongdou
