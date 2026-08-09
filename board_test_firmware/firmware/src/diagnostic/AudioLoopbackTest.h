#pragma once

#include <Arduino.h>

#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"

namespace tongdou {

class AudioLoopbackTest {
 public:
  AudioLoopbackTest(AudioInput& audioInput, AudioOutput& audioOutput);

  void update();
  void start(Print& out);
  void stop(Print& out);
  void printStatus(Print& out) const;
  bool active() const;
  bool recordingAvailable() const;
  uint32_t sampleRateHz() const;
  const int16_t* recordedSamples() const;
  size_t recordedSampleCount() const;
  size_t recordedByteCount() const;
  int32_t livePeak() const;
  int32_t liveAverageAbs() const;

 private:
  enum class Phase : uint8_t {
    Idle,
    Recording,
    Complete,
    Failed,
  };

  void finishRecording();
  void fail(const char* message);
  void analyzeRecording();
  void releaseBuffer();
  const char* phaseName() const;

  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  int16_t* samples_ = nullptr;
  Phase phase_ = Phase::Idle;
  unsigned long recordingStartedMs_ = 0;
  size_t recordedSamples_ = 0;
  int32_t mean_ = 0;
  int32_t peak_ = 0;
  int32_t averageAbs_ = 0;
  int32_t livePeak_ = 0;
  int32_t liveAverageAbs_ = 0;
  size_t clippedSamples_ = 0;
  uint8_t warmupReadsRemaining_ = 0;
  bool signalDetected_ = false;
  bool captureOk_ = false;
  char message_[64] = "idle";
};

}  // namespace tongdou
