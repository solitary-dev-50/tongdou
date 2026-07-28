#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace tongdou {

class AudioOutput {
 public:
  void begin();
  void playTestTone(uint16_t frequencyHz = 880, uint16_t durationMs = 120);
  void playClip(uint16_t clipId, const char* text);
  void playClip(uint16_t clipId, const char* fileName, const char* text);
  bool writePcm16Mono(const int16_t* samples, size_t sampleCount, uint32_t sampleRate);
  void stopStream();
  bool ready() const;
  uint32_t lastPlaybackStartedAtMs() const;
  uint8_t volumePercent() const;
  void setVolumePercent(uint8_t percent);

 private:
  static void playbackTaskEntry(void* context);
  void playbackTask();
  bool startPlaybackTask(const char* fileName);

  bool ready_ = false;
  bool fsReady_ = false;
  volatile bool stopRequested_ = false;
  volatile bool playbackRunning_ = false;
  volatile bool playbackStartPending_ = false;
  volatile uint32_t playbackStartedAtMs_ = 0;
  TaskHandle_t playbackTask_ = nullptr;
  char playbackFileName_[96] = {};
  uint32_t currentSampleRate_ = 0;
  uint8_t volumePercent_ = 80;
};

}  // namespace tongdou
