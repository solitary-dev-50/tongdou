#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "audio/PcmAudioSink.h"

extern "C" {
#include "esp_mn_iface.h"
#include "model_path.h"
}

namespace tongdou {

struct LocalWakeWordSnapshot {
  bool ready = false;
  bool armed = false;
  uint32_t samplesOffered = 0;
  uint32_t samplesDropped = 0;
  uint32_t detections = 0;
  size_t modelFrameSamples = 0;
  size_t psramUsedBytes = 0;
  char lastError[64] = {};
};

class LocalWakeWordDetector : public PcmAudioSink {
 public:
  void begin();
  void setArmed(bool armed);
  bool consumeDetectedEvent();
  LocalWakeWordSnapshot snapshot() const;

  void offerPcm16(const int16_t* samples, size_t sampleCount) override;

 private:
  static constexpr size_t kQueueCapacitySamples = 16000;
  static constexpr uint32_t kModelPartitionMaxCount = 8;

  static void detectorTaskEntry(void* context);
  void detectorTaskLoop();
  bool initializeModel();
  void releaseModel();
  size_t popFrame(int16_t* destination, size_t sampleCount);
  void setError(const char* error);

  mutable portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
  TaskHandle_t taskHandle_ = nullptr;
  int16_t* queue_ = nullptr;
  size_t queueReadIndex_ = 0;
  size_t queueWriteIndex_ = 0;
  size_t queueCount_ = 0;
  bool receivedPcmSinceArmed_ = false;
  volatile bool armed_ = false;
  volatile bool detectedEventPending_ = false;
  LocalWakeWordSnapshot snapshot_;
  srmodel_list_t* models_ = nullptr;
  esp_mn_iface_t* multinet_ = nullptr;
  model_iface_data_t* modelData_ = nullptr;
  int16_t* modelFrame_ = nullptr;
};

}  // namespace tongdou
