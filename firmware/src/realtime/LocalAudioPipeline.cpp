#include "realtime/LocalAudioPipeline.h"

#include <esp_heap_caps.h>

namespace tongdou {
namespace {

constexpr uint32_t kAudioTaskStackWords = 4096;
constexpr UBaseType_t kAudioTaskPriority = 3;

}  // namespace

LocalAudioPipeline::LocalAudioPipeline(AudioInput& audioInput)
    : audioInput_(audioInput) {}

void LocalAudioPipeline::begin() {
  if (mutex_ == nullptr) {
    mutex_ = xSemaphoreCreateMutex();
  }

  snapshot_ = {};
  if (uploadQueue_ == nullptr) {
    uploadQueue_ = static_cast<int16_t*>(heap_caps_malloc(
        kUploadQueueCapacitySamples * sizeof(int16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  snapshot_.uploadBufferReady = uploadQueue_ != nullptr;
  snapshot_.uploadBufferCapacitySamples =
      uploadQueue_ != nullptr ? kUploadQueueCapacitySamples : 0;
  snapshot_.uploadBufferPsramBytes =
      uploadQueue_ != nullptr
          ? kUploadQueueCapacitySamples * sizeof(int16_t)
          : 0;
  snapshot_.ready = audioInput_.ready() && snapshot_.uploadBufferReady;
  Serial.print(F("local audio upload buffer ready="));
  Serial.print(snapshot_.uploadBufferReady ? 1 : 0);
  Serial.print(F(" samples="));
  Serial.print(snapshot_.uploadBufferCapacitySamples);
  Serial.print(F(" psram_bytes="));
  Serial.println(snapshot_.uploadBufferPsramBytes);
  if (taskHandle_ == nullptr) {
    const BaseType_t result = xTaskCreatePinnedToCore(
        audioTaskEntry, "local_audio", kAudioTaskStackWords, this,
        kAudioTaskPriority, &taskHandle_, 0);
    if (result != pdPASS) {
      taskHandle_ = nullptr;
      Serial.println(F("local audio task start failed"));
    }
  }
}

void LocalAudioPipeline::update() {
  if (mutex_ == nullptr) {
    return;
  }
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.ready = audioInput_.ready() && snapshot_.uploadBufferReady;
  xSemaphoreGive(mutex_);
}

bool LocalAudioPipeline::ready() const {
  return snapshot_.ready;
}

LocalAudioPipelineSnapshot LocalAudioPipeline::snapshot() const {
  if (mutex_ == nullptr) {
    return snapshot_;
  }
  xSemaphoreTake(mutex_, portMAX_DELAY);
  LocalAudioPipelineSnapshot copy = snapshot_;
  copy.uploadSamplesQueued = static_cast<uint32_t>(uploadCount_);
  xSemaphoreGive(mutex_);
  return copy;
}

void LocalAudioPipeline::clearUploadQueue() {
  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  uploadReadIndex_ = 0;
  uploadWriteIndex_ = 0;
  uploadCount_ = 0;
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
}

void LocalAudioPipeline::startUpload() {
  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  snapshot_.uploadActive = true;
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
}

void LocalAudioPipeline::stopUpload() {
  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  snapshot_.uploadActive = false;
  uploadReadIndex_ = 0;
  uploadWriteIndex_ = 0;
  uploadCount_ = 0;
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
}

bool LocalAudioPipeline::readUploadFrame(int16_t* buffer, size_t frameSamples) {
  if (buffer == nullptr || frameSamples == 0 || uploadQueue_ == nullptr) {
    return false;
  }

  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  if (uploadCount_ < frameSamples) {
    if (mutex_ != nullptr) {
      xSemaphoreGive(mutex_);
    }
    return false;
  }
  for (size_t i = 0; i < frameSamples; ++i) {
    buffer[i] = uploadQueue_[uploadReadIndex_];
    uploadReadIndex_ = (uploadReadIndex_ + 1) % kUploadQueueCapacitySamples;
  }
  uploadCount_ -= frameSamples;
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
  return true;
}

size_t LocalAudioPipeline::readUploadSamples(int16_t* buffer, size_t maxSamples) {
  if (buffer == nullptr || maxSamples == 0 || uploadQueue_ == nullptr) {
    return 0;
  }

  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  if (uploadCount_ == 0) {
    if (mutex_ != nullptr) {
      xSemaphoreGive(mutex_);
    }
    return 0;
  }
  const size_t samplesToRead = min(maxSamples, uploadCount_);
  for (size_t i = 0; i < samplesToRead; ++i) {
    buffer[i] = uploadQueue_[uploadReadIndex_];
    uploadReadIndex_ = (uploadReadIndex_ + 1) % kUploadQueueCapacitySamples;
  }
  uploadCount_ -= samplesToRead;
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
  return samplesToRead;
}

void LocalAudioPipeline::setMonitor(PcmAudioSink* monitor) {
  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  monitor_ = monitor;
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
}

void LocalAudioPipeline::audioTaskEntry(void* context) {
  static_cast<LocalAudioPipeline*>(context)->audioTaskLoop();
}

void LocalAudioPipeline::audioTaskLoop() {
  while (true) {
    if (!audioInput_.ready()) {
      if (mutex_ != nullptr) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        snapshot_.ready = false;
        xSemaphoreGive(mutex_);
      }
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    int16_t samples[kReadFrameSamples] = {};
    size_t samplesRead = 0;
    if (!audioInput_.readPcm16(samples, kReadFrameSamples, samplesRead) ||
        samplesRead == 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    PcmAudioSink* monitor = nullptr;
    if (mutex_ != nullptr) {
      xSemaphoreTake(mutex_, portMAX_DELAY);
    }
    snapshot_.ready = uploadQueue_ != nullptr;
    snapshot_.samplesRead += static_cast<uint32_t>(samplesRead);
    if (snapshot_.uploadActive) {
      storeUploadSamples(samples, samplesRead);
    }
    monitor = monitor_;
    if (mutex_ != nullptr) {
      xSemaphoreGive(mutex_);
    }
    if (monitor != nullptr) {
      monitor->offerPcm16(samples, samplesRead);
    }
  }
}

void LocalAudioPipeline::storeUploadSamples(const int16_t* samples,
                                            size_t sampleCount) {
  if (samples == nullptr || sampleCount == 0) {
    return;
  }
  if (uploadQueue_ == nullptr) {
    return;
  }

  for (size_t i = 0; i < sampleCount; ++i) {
    if (uploadCount_ >= kUploadQueueCapacitySamples) {
      uploadReadIndex_ = (uploadReadIndex_ + 1) % kUploadQueueCapacitySamples;
      --uploadCount_;
      ++snapshot_.uploadSamplesDropped;
    }
    uploadQueue_[uploadWriteIndex_] = samples[i];
    uploadWriteIndex_ = (uploadWriteIndex_ + 1) % kUploadQueueCapacitySamples;
    ++uploadCount_;
  }
}

}  // namespace tongdou
