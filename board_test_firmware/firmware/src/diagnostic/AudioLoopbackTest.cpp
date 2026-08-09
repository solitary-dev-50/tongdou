#include "diagnostic/AudioLoopbackTest.h"

#include <stdlib.h>
#include <string.h>

namespace tongdou {
namespace {

constexpr uint32_t kSampleRate = 16000;
constexpr uint32_t kRecordDurationMs = 3000;
constexpr uint32_t kRecordTimeoutMs = 5000;
constexpr size_t kTargetSamples =
    static_cast<size_t>(kSampleRate) * kRecordDurationMs / 1000U;
constexpr size_t kChunkSamples = 128;
constexpr uint8_t kWarmupReads = 6;
constexpr int32_t kSignalPeakThreshold = 300;
constexpr int32_t kSignalAverageThreshold = 20;
constexpr int32_t kClipThreshold = 32000;

int32_t absoluteValue(int32_t value) {
  return value < 0 ? -value : value;
}

void analyzeChunk(const int16_t* samples, size_t sampleCount,
                  int32_t& peak, int32_t& averageAbs) {
  peak = 0;
  averageAbs = 0;
  if (samples == nullptr || sampleCount == 0) {
    return;
  }

  int64_t sum = 0;
  for (size_t index = 0; index < sampleCount; ++index) {
    sum += samples[index];
  }
  const int32_t mean =
      static_cast<int32_t>(sum / static_cast<int64_t>(sampleCount));

  int64_t absoluteSum = 0;
  for (size_t index = 0; index < sampleCount; ++index) {
    const int32_t centered = static_cast<int32_t>(samples[index]) - mean;
    const int32_t magnitude = absoluteValue(centered);
    absoluteSum += magnitude;
    if (magnitude > peak) {
      peak = magnitude;
    }
  }
  averageAbs =
      static_cast<int32_t>(absoluteSum / static_cast<int64_t>(sampleCount));
}

}  // namespace

AudioLoopbackTest::AudioLoopbackTest(AudioInput& audioInput, AudioOutput& audioOutput)
    : audioInput_(audioInput), audioOutput_(audioOutput) {}

void AudioLoopbackTest::update() {
  if (phase_ == Phase::Recording) {
    if (millis() - recordingStartedMs_ >= kRecordTimeoutMs) {
      fail("recording timeout");
      return;
    }

    int16_t warmupSamples[kChunkSamples] = {};
    if (warmupReadsRemaining_ > 0) {
      size_t discardedSamples = 0;
      if (audioInput_.readPcm16(warmupSamples, kChunkSamples,
                                discardedSamples)) {
        --warmupReadsRemaining_;
      }
      return;
    }

    const size_t remaining = kTargetSamples - recordedSamples_;
    size_t samplesRead = 0;
    if (audioInput_.readPcm16(samples_ + recordedSamples_,
                              min(remaining, kChunkSamples), samplesRead)) {
      analyzeChunk(samples_ + recordedSamples_, samplesRead, livePeak_,
                   liveAverageAbs_);
      recordedSamples_ += samplesRead;
    }

    if (recordedSamples_ >= kTargetSamples) {
      finishRecording();
    }
    return;
  }
}

void AudioLoopbackTest::start(Print& out) {
  if (active()) {
    printStatus(out);
    return;
  }

  releaseBuffer();
  recordedSamples_ = 0;
  mean_ = 0;
  peak_ = 0;
  averageAbs_ = 0;
  livePeak_ = 0;
  liveAverageAbs_ = 0;
  clippedSamples_ = 0;
  warmupReadsRemaining_ = kWarmupReads;
  signalDetected_ = false;
  captureOk_ = false;

  if (!audioInput_.ready()) {
    fail("microphone not ready");
    printStatus(out);
    return;
  }

  samples_ = static_cast<int16_t*>(malloc(kTargetSamples * sizeof(int16_t)));
  if (samples_ == nullptr) {
    fail("record buffer allocation failed");
    printStatus(out);
    return;
  }

  phase_ = Phase::Recording;
  recordingStartedMs_ = millis();
  strncpy(message_, "speak now", sizeof(message_) - 1);
  message_[sizeof(message_) - 1] = '\0';

  Serial.println(F("audio record started"));
  Serial.print(F("  sample_rate_hz="));
  Serial.println(kSampleRate);
  Serial.println(F("  bits=16"));
  Serial.println(F("  channels=1"));
  Serial.print(F("  duration_ms="));
  Serial.println(kRecordDurationMs);
  Serial.print(F("  warmup_reads="));
  Serial.println(kWarmupReads);
  Serial.print(F("  buffer_bytes="));
  Serial.println(kTargetSamples * sizeof(int16_t));
  Serial.println(F("  instruction=speak now"));
  printStatus(out);
}

void AudioLoopbackTest::stop(Print& out) {
  if (active()) {
    phase_ = Phase::Idle;
    strncpy(message_, "stopped", sizeof(message_) - 1);
    message_[sizeof(message_) - 1] = '\0';
    releaseBuffer();
    Serial.println(F("audio record stopped"));
  }
  printStatus(out);
}

void AudioLoopbackTest::printStatus(Print& out) const {
  out.println(F("audio record:"));
  out.print(F("  active="));
  out.println(active() ? 1 : 0);
  out.print(F("  done="));
  out.println(phase_ == Phase::Complete ? 1 : 0);
  out.print(F("  failed="));
  out.println(phase_ == Phase::Failed ? 1 : 0);
  out.print(F("  phase="));
  out.println(phaseName());
  out.print(F("  message="));
  out.println(message_);
  out.print(F("  sample_rate_hz="));
  out.println(kSampleRate);
  out.println(F("  bits=16"));
  out.println(F("  channels=1"));
  out.print(F("  target_duration_ms="));
  out.println(kRecordDurationMs);
  out.print(F("  warmup_reads_remaining="));
  out.println(warmupReadsRemaining_);
  out.print(F("  recorded_samples="));
  out.println(recordedSamples_);
  out.print(F("  recorded_bytes="));
  out.println(recordedSamples_ * sizeof(int16_t));
  out.print(F("  mean="));
  out.println(mean_);
  out.print(F("  peak="));
  out.println(peak_);
  out.print(F("  avg_abs="));
  out.println(averageAbs_);
  out.print(F("  live_peak="));
  out.println(livePeak_);
  out.print(F("  live_avg_abs="));
  out.println(liveAverageAbs_);
  out.print(F("  clipped_samples="));
  out.println(clippedSamples_);
  out.print(F("  signal_detected="));
  out.println(signalDetected_ ? 1 : 0);
  out.print(F("  capture_ok="));
  out.println(captureOk_ ? 1 : 0);
  out.print(F("  download_path="));
  out.println(recordingAvailable() ? F("/api/audio/recording.wav") : F(""));
  if (phase_ == Phase::Complete) {
    out.println(F("  verdict=download the WAV file and listen to the recording"));
  }
}

bool AudioLoopbackTest::active() const {
  return phase_ == Phase::Recording;
}

bool AudioLoopbackTest::recordingAvailable() const {
  return phase_ == Phase::Complete && samples_ != nullptr && recordedSamples_ > 0;
}

uint32_t AudioLoopbackTest::sampleRateHz() const {
  return kSampleRate;
}

const int16_t* AudioLoopbackTest::recordedSamples() const {
  return samples_;
}

size_t AudioLoopbackTest::recordedSampleCount() const {
  return recordedSamples_;
}

size_t AudioLoopbackTest::recordedByteCount() const {
  return recordedSamples_ * sizeof(int16_t);
}

int32_t AudioLoopbackTest::livePeak() const {
  return livePeak_;
}

int32_t AudioLoopbackTest::liveAverageAbs() const {
  return liveAverageAbs_;
}

void AudioLoopbackTest::finishRecording() {
  analyzeRecording();
  phase_ = Phase::Complete;
  strncpy(message_, "recording complete, download WAV", sizeof(message_) - 1);
  message_[sizeof(message_) - 1] = '\0';

  Serial.print(F("audio record complete samples="));
  Serial.print(recordedSamples_);
  Serial.print(F(" peak="));
  Serial.print(peak_);
  Serial.print(F(" avg_abs="));
  Serial.print(averageAbs_);
  Serial.print(F(" clipped="));
  Serial.print(clippedSamples_);
  Serial.print(F(" signal_detected="));
  Serial.println(signalDetected_ ? 1 : 0);
  Serial.print(F("  capture_ok="));
  Serial.println(captureOk_ ? 1 : 0);
  Serial.println(F("  download_path=/api/audio/recording.wav"));
}

void AudioLoopbackTest::fail(const char* message) {
  phase_ = Phase::Failed;
  strncpy(message_, message, sizeof(message_) - 1);
  message_[sizeof(message_) - 1] = '\0';
  releaseBuffer();

  Serial.print(F("audio record failed: "));
  Serial.println(message_);
}

void AudioLoopbackTest::analyzeRecording() {
  if (samples_ == nullptr || recordedSamples_ == 0) {
    return;
  }

  int64_t sum = 0;
  for (size_t index = 0; index < recordedSamples_; ++index) {
    sum += samples_[index];
    if (absoluteValue(samples_[index]) >= kClipThreshold) {
      ++clippedSamples_;
    }
  }
  mean_ = static_cast<int32_t>(sum / static_cast<int64_t>(recordedSamples_));

  int64_t absoluteSum = 0;
  for (size_t index = 0; index < recordedSamples_; ++index) {
    const int32_t centered = static_cast<int32_t>(samples_[index]) - mean_;
    const int32_t magnitude = absoluteValue(centered);
    absoluteSum += magnitude;
    if (magnitude > peak_) {
      peak_ = magnitude;
    }
  }
  averageAbs_ =
      static_cast<int32_t>(absoluteSum / static_cast<int64_t>(recordedSamples_));
  signalDetected_ =
      peak_ >= kSignalPeakThreshold && averageAbs_ >= kSignalAverageThreshold;
  captureOk_ = recordedSamples_ == kTargetSamples && signalDetected_;
}

void AudioLoopbackTest::releaseBuffer() {
  if (samples_ != nullptr) {
    free(samples_);
    samples_ = nullptr;
  }
}

const char* AudioLoopbackTest::phaseName() const {
  switch (phase_) {
    case Phase::Idle:
      return "idle";
    case Phase::Recording:
      return "recording";
    case Phase::Complete:
      return "complete";
    case Phase::Failed:
      return "failed";
  }
  return "unknown";
}

}  // namespace tongdou
