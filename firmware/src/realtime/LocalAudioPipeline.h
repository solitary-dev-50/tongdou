#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "audio/PcmAudioSink.h"
#include "hardware/AudioInput.h"

namespace tongdou {

struct LocalAudioPipelineSnapshot {
  bool ready = false;
  bool uploadActive = false;
  bool uploadBufferReady = false;
  uint32_t samplesRead = 0;
  uint32_t uploadSamplesQueued = 0;
  uint32_t uploadSamplesDropped = 0;
  uint32_t uploadBufferCapacitySamples = 0;
  uint32_t uploadBufferPsramBytes = 0;
};

class LocalAudioPipeline {
 public:
  static constexpr size_t kUploadFrameSamples = 960;

  explicit LocalAudioPipeline(AudioInput& audioInput);

  void begin();
  void update();

  bool ready() const;
  LocalAudioPipelineSnapshot snapshot() const;

  void clearUploadQueue();
  void startUpload();
  void stopUpload();
  bool readUploadFrame(int16_t* buffer, size_t frameSamples);
  size_t readUploadSamples(int16_t* buffer, size_t maxSamples);
  void setMonitor(PcmAudioSink* monitor);

 private:
  static constexpr size_t kReadFrameSamples = 160;
  static constexpr size_t kUploadQueueSeconds = 6;
  static constexpr size_t kUploadQueueCapacitySamples =
      16000 * kUploadQueueSeconds;

  static void audioTaskEntry(void* context);
  void audioTaskLoop();
  void storeUploadSamples(const int16_t* samples, size_t sampleCount);

  AudioInput& audioInput_;
  SemaphoreHandle_t mutex_ = nullptr;
  TaskHandle_t taskHandle_ = nullptr;
  LocalAudioPipelineSnapshot snapshot_;
  int16_t* uploadQueue_ = nullptr;
  size_t uploadReadIndex_ = 0;
  size_t uploadWriteIndex_ = 0;
  size_t uploadCount_ = 0;
  PcmAudioSink* monitor_ = nullptr;
};

}  // namespace tongdou
