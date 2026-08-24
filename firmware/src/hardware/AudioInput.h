#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace tongdou {

struct AudioInputSnapshot {
  bool ready = false;
  bool hasSamples = false;
  size_t samplesRead = 0;
  int32_t mean = 0;
  int32_t minSample = 0;
  int32_t maxSample = 0;
  int32_t peak = 0;
  int32_t averageAbs = 0;
};

class AudioInput {
 public:
  void begin();
  void update();
  AudioInputSnapshot readLevel();
  bool readPcm16(int16_t* buffer, size_t maxSamples, size_t& samplesRead);
  bool ready() const;

 private:
  SemaphoreHandle_t mutex_ = nullptr;
  bool ready_ = false;
};

}  // namespace tongdou
