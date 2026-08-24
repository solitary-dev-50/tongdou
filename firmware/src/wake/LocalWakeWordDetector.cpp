#include "wake/LocalWakeWordDetector.h"

#include <cstring>

#include <esp_heap_caps.h>
#include <esp_partition.h>

extern "C" {
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "model_path.h"
}

namespace tongdou {
namespace {

constexpr char kModelPartitionLabel[] = "model";
constexpr char kWakeModelName[] = "mn4q8_cn";
constexpr char kWakeCommandPinyin[] = "ni hao tong dou";
constexpr int kWakeCommandId = 1;
constexpr int kRecognitionTimeoutMs = 3000;
constexpr float kDetectionThreshold = 0.20F;
constexpr uint32_t kDetectorTaskStackWords = 6144;
constexpr UBaseType_t kDetectorTaskPriority = 2;

void printWakeMemory(const char* phase) {
  Serial.print(F("wake memory phase="));
  Serial.print(phase);
  Serial.print(F(" psram_free="));
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  Serial.print(F(" psram_largest="));
  Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
  Serial.print(F(" internal_free="));
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  Serial.print(F(" internal_largest="));
  Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

}  // namespace

void LocalWakeWordDetector::begin() {
  snapshot_ = {};
  detectedEventPending_ = false;
  armed_ = false;
  printWakeMemory("before");

  queue_ = static_cast<int16_t*>(heap_caps_malloc(
      kQueueCapacitySamples * sizeof(int16_t),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (queue_ == nullptr) {
    setError("wake_queue_psram_allocation_failed");
    printWakeMemory("failed");
    return;
  }
  snapshot_.psramUsedBytes = kQueueCapacitySamples * sizeof(int16_t);

  if (!initializeModel()) {
    releaseModel();
    printWakeMemory("failed");
    return;
  }

  const BaseType_t taskResult = xTaskCreatePinnedToCore(
      detectorTaskEntry, "wake_word", kDetectorTaskStackWords, this,
      kDetectorTaskPriority, &taskHandle_, 1);
  if (taskResult != pdPASS) {
    taskHandle_ = nullptr;
    setError("wake_detector_task_start_failed");
    releaseModel();
    printWakeMemory("failed");
    return;
  }

  portENTER_CRITICAL(&lock_);
  snapshot_.ready = true;
  snapshot_.lastError[0] = '\0';
  portEXIT_CRITICAL(&lock_);
  Serial.print(F("wake model init locale=zh-CN model="));
  Serial.print(kWakeModelName);
  Serial.println(F(" ok=1"));
  printWakeMemory("after");
  Serial.print(F("wake detector ready phrase=你好，铜豆 frame_samples="));
  Serial.print(snapshot_.modelFrameSamples);
  Serial.print(F(" psram_queue_bytes="));
  Serial.println(snapshot_.psramUsedBytes);
}

void LocalWakeWordDetector::setArmed(bool armed) {
  if (!snapshot_.ready) {
    return;
  }

  portENTER_CRITICAL(&lock_);
  const bool stateChanged = armed_ != armed;
  armed_ = armed;
  snapshot_.armed = armed;
  if (stateChanged) {
    receivedPcmSinceArmed_ = false;
  }
  if (!armed) {
    detectedEventPending_ = false;
    queueReadIndex_ = 0;
    queueWriteIndex_ = 0;
    queueCount_ = 0;
  }
  portEXIT_CRITICAL(&lock_);
  if (stateChanged) {
    Serial.print(F("wake detector armed="));
    Serial.println(armed ? "1" : "0");
  }
}

bool LocalWakeWordDetector::consumeDetectedEvent() {
  portENTER_CRITICAL(&lock_);
  const bool pending = detectedEventPending_;
  detectedEventPending_ = false;
  portEXIT_CRITICAL(&lock_);
  return pending;
}

LocalWakeWordSnapshot LocalWakeWordDetector::snapshot() const {
  portENTER_CRITICAL(&lock_);
  const LocalWakeWordSnapshot copy = snapshot_;
  portEXIT_CRITICAL(&lock_);
  return copy;
}

void LocalWakeWordDetector::offerPcm16(const int16_t* samples,
                                       size_t sampleCount) {
  if (!armed_ || samples == nullptr || sampleCount == 0 || queue_ == nullptr) {
    return;
  }

  portENTER_CRITICAL(&lock_);
  const bool logFirstPcm = !receivedPcmSinceArmed_;
  receivedPcmSinceArmed_ = true;
  snapshot_.samplesOffered += static_cast<uint32_t>(sampleCount);
  for (size_t i = 0; i < sampleCount; ++i) {
    if (queueCount_ >= kQueueCapacitySamples) {
      ++snapshot_.samplesDropped;
      continue;
    }
    queue_[queueWriteIndex_] = samples[i];
    queueWriteIndex_ = (queueWriteIndex_ + 1) % kQueueCapacitySamples;
    ++queueCount_;
  }
  portEXIT_CRITICAL(&lock_);
  if (logFirstPcm) {
    Serial.print(F("wake detector first_pcm samples="));
    Serial.println(sampleCount);
  }
}

void LocalWakeWordDetector::detectorTaskEntry(void* context) {
  static_cast<LocalWakeWordDetector*>(context)->detectorTaskLoop();
}

void LocalWakeWordDetector::detectorTaskLoop() {
  bool wasArmed = false;
  while (true) {
    if (!armed_) {
      if (wasArmed && multinet_ != nullptr && modelData_ != nullptr) {
        multinet_->clean(modelData_);
      }
      wasArmed = false;
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    wasArmed = true;

    const size_t frameSamples = snapshot_.modelFrameSamples;
    if (popFrame(modelFrame_, frameSamples) != frameSamples) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    const esp_mn_state_t state = multinet_->detect(modelData_, modelFrame_);
    if (state == ESP_MN_STATE_TIMEOUT) {
      multinet_->clean(modelData_);
      continue;
    }
    if (state != ESP_MN_STATE_DETECTED) {
      continue;
    }

    const esp_mn_results_t* results = multinet_->get_results(modelData_);
    if (results == nullptr || results->num == 0 ||
        results->command_id[0] != kWakeCommandId) {
      continue;
    }

    const float probability = results->prob[0];
    portENTER_CRITICAL(&lock_);
    ++snapshot_.detections;
    detectedEventPending_ = true;
    armed_ = false;
    snapshot_.armed = false;
    queueReadIndex_ = 0;
    queueWriteIndex_ = 0;
    queueCount_ = 0;
    portEXIT_CRITICAL(&lock_);
    multinet_->clean(modelData_);
    wasArmed = false;
    Serial.print(F("wake word detected phrase=你好，铜豆 probability="));
    Serial.println(probability, 3);
  }
}

bool LocalWakeWordDetector::initializeModel() {
  const esp_partition_t* partition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,
      kModelPartitionLabel);
  if (partition == nullptr) {
    setError("wake_model_partition_missing");
    return false;
  }

  uint32_t modelCount = 0;
  if (esp_partition_read(partition, 0, &modelCount, sizeof(modelCount)) !=
          ESP_OK ||
      modelCount == 0 || modelCount > kModelPartitionMaxCount) {
    setError("wake_model_image_missing");
    return false;
  }

  models_ = esp_srmodel_init(kModelPartitionLabel);
  if (models_ == nullptr) {
    setError("wake_model_load_failed");
    return false;
  }

  char* modelName =
      esp_srmodel_filter(models_, ESP_MN_PREFIX, ESP_MN_CHINESE);
  if (modelName == nullptr) {
    setError("wake_chinese_model_missing");
    return false;
  }

  multinet_ = esp_mn_handle_from_name(modelName);
  if (multinet_ == nullptr) {
    setError("wake_multinet_handle_failed");
    return false;
  }

  modelData_ = multinet_->create(modelName, kRecognitionTimeoutMs);
  if (modelData_ == nullptr) {
    setError("wake_multinet_create_failed");
    return false;
  }

  if (esp_mn_commands_clear() != ESP_OK ||
      esp_mn_commands_add(kWakeCommandId,
                          const_cast<char*>(kWakeCommandPinyin)) != ESP_OK ||
      esp_mn_commands_update() != nullptr) {
    setError("wake_command_configuration_failed");
    return false;
  }
  multinet_->set_det_threshold(modelData_, kDetectionThreshold);

  const int frameSamples = multinet_->get_samp_chunksize(modelData_);
  if (frameSamples <= 0) {
    setError("wake_model_frame_size_invalid");
    return false;
  }
  modelFrame_ = static_cast<int16_t*>(heap_caps_malloc(
      static_cast<size_t>(frameSamples) * sizeof(int16_t),
      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (modelFrame_ == nullptr) {
    setError("wake_model_frame_allocation_failed");
    return false;
  }
  snapshot_.modelFrameSamples = static_cast<size_t>(frameSamples);
  return true;
}

void LocalWakeWordDetector::releaseModel() {
  if (modelData_ != nullptr && multinet_ != nullptr) {
    multinet_->destroy(modelData_);
  }
  modelData_ = nullptr;
  multinet_ = nullptr;
  if (models_ != nullptr) {
    esp_srmodel_deinit(models_);
  }
  models_ = nullptr;
  if (modelFrame_ != nullptr) {
    heap_caps_free(modelFrame_);
  }
  modelFrame_ = nullptr;
  if (queue_ != nullptr) {
    heap_caps_free(queue_);
  }
  queue_ = nullptr;
  snapshot_.ready = false;
}

size_t LocalWakeWordDetector::popFrame(int16_t* destination,
                                       size_t sampleCount) {
  if (destination == nullptr || sampleCount == 0) {
    return 0;
  }

  portENTER_CRITICAL(&lock_);
  if (queueCount_ < sampleCount) {
    portEXIT_CRITICAL(&lock_);
    return 0;
  }
  for (size_t i = 0; i < sampleCount; ++i) {
    destination[i] = queue_[queueReadIndex_];
    queueReadIndex_ = (queueReadIndex_ + 1) % kQueueCapacitySamples;
  }
  queueCount_ -= sampleCount;
  portEXIT_CRITICAL(&lock_);
  return sampleCount;
}

void LocalWakeWordDetector::setError(const char* error) {
  const char* safeError = error == nullptr ? "unknown" : error;
  portENTER_CRITICAL(&lock_);
  snapshot_.ready = false;
  strncpy(snapshot_.lastError, safeError, sizeof(snapshot_.lastError) - 1);
  snapshot_.lastError[sizeof(snapshot_.lastError) - 1] = '\0';
  portEXIT_CRITICAL(&lock_);
  Serial.print(F("wake detector unavailable error="));
  Serial.println(safeError);
  Serial.print(F("wake model init locale=zh-CN model="));
  Serial.print(kWakeModelName);
  Serial.print(F(" ok=0 code="));
  Serial.println(safeError);
}

}  // namespace tongdou
