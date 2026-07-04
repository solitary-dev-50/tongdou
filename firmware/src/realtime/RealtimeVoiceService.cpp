#include "realtime/RealtimeVoiceService.h"

namespace tongdou {
namespace {

constexpr uint16_t kMinCaptureMs = 200;
constexpr uint16_t kMaxCaptureMs = 4000;
constexpr unsigned long kFrameIntervalMs = 20;
constexpr size_t kFrameSamples = 128;
constexpr uint16_t kDefaultResponseClipId = 200;

uint16_t clampCaptureMs(uint16_t captureMs) {
  if (captureMs < kMinCaptureMs) {
    return kMinCaptureMs;
  }
  if (captureMs > kMaxCaptureMs) {
    return kMaxCaptureMs;
  }
  return captureMs;
}

}  // namespace

RealtimeVoiceService::RealtimeVoiceService(AudioInput& audioInput,
                                           AudioOutput& audioOutput)
    : audioInput_(audioInput), audioOutput_(audioOutput) {}

void RealtimeVoiceService::begin() {
  sender_.begin();
  snapshot_ = {};
  snapshot_.state = RealtimeVoiceState::Idle;
  snapshot_.audioInputReady = audioInput_.ready();
  snapshot_.audioOutputReady = audioOutput_.ready();
}

void RealtimeVoiceService::update() {
  snapshot_.audioInputReady = audioInput_.ready();
  snapshot_.audioOutputReady = audioOutput_.ready();

  if (snapshot_.state != RealtimeVoiceState::Listening) {
    return;
  }

  const unsigned long now = millis();
  if (static_cast<long>(now - captureUntilMs_) >= 0) {
    finishCapture();
    return;
  }
  if (static_cast<long>(now - nextCaptureMs_) < 0) {
    return;
  }

  captureFrame(now);
}

bool RealtimeVoiceService::startTurn(uint16_t captureMs) {
  if (snapshot_.state == RealtimeVoiceState::Listening) {
    failTurn("voice_busy", "voice turn is already listening");
    return false;
  }
  if (!audioInput_.ready()) {
    failTurn("audio_input_not_ready", "microphone is not ready");
    return false;
  }

  sender_.reset();
  ++snapshot_.turnId;
  snapshot_.state = RealtimeVoiceState::Listening;
  snapshot_.lastTurnOk = false;
  snapshot_.lastErrorCode = "";
  snapshot_.lastErrorMessage = "";
  snapshot_.chunksSent = 0;
  snapshot_.samplesSent = 0;
  snapshot_.bytesSent = 0;

  const unsigned long now = millis();
  captureUntilMs_ = now + clampCaptureMs(captureMs);
  nextCaptureMs_ = now;
  perfTracer_.listenStart();
  return true;
}

bool RealtimeVoiceService::playResponse(uint16_t clipId, const String& text) {
  if (!audioOutput_.ready()) {
    failTurn("audio_output_not_ready", "speaker is not ready");
    return false;
  }

  const uint16_t safeClipId = clipId == 0 ? kDefaultResponseClipId : clipId;
  audioOutput_.playClip(safeClipId, text.c_str());
  snapshot_.state = RealtimeVoiceState::Idle;
  snapshot_.lastTurnOk = true;
  snapshot_.lastErrorCode = "";
  snapshot_.lastErrorMessage = "";
  return true;
}

void RealtimeVoiceService::failTurn(const String& code, const String& message) {
  snapshot_.state = RealtimeVoiceState::Failed;
  snapshot_.lastTurnOk = false;
  snapshot_.lastErrorCode = code;
  snapshot_.lastErrorMessage = message;
  failurePending_ = true;
}

bool RealtimeVoiceService::consumeFailureEvent() {
  if (!failurePending_) {
    return false;
  }

  failurePending_ = false;
  return true;
}

RealtimeVoiceSnapshot RealtimeVoiceService::snapshot() const {
  RealtimeVoiceSnapshot copy = snapshot_;
  const RealtimeAudioSendStats stats = sender_.stats();
  copy.chunksSent = stats.chunksSent;
  copy.samplesSent = stats.samplesSent;
  copy.bytesSent = stats.bytesSent;
  copy.audioInputReady = audioInput_.ready();
  copy.audioOutputReady = audioOutput_.ready();
  return copy;
}

const char* RealtimeVoiceService::stateName() const {
  switch (snapshot_.state) {
    case RealtimeVoiceState::Listening:
      return "listening";
    case RealtimeVoiceState::AwaitingResponse:
      return "awaiting_response";
    case RealtimeVoiceState::Failed:
      return "failed";
    case RealtimeVoiceState::Idle:
    default:
      return "idle";
  }
}

void RealtimeVoiceService::captureFrame(unsigned long nowMs) {
  int16_t samples[kFrameSamples] = {};
  size_t samplesRead = 0;
  if (audioInput_.readPcm16(samples, kFrameSamples, samplesRead)) {
    if (sender_.sendPcm16(samples, samplesRead)) {
      perfTracer_.firstAudioSent();
    }
  }

  nextCaptureMs_ = nowMs + kFrameIntervalMs;
}

void RealtimeVoiceService::finishCapture() {
  const RealtimeAudioSendStats stats = sender_.stats();
  if (stats.samplesSent == 0) {
    failTurn("audio_capture_empty", "microphone returned no samples");
    return;
  }

  snapshot_.state = RealtimeVoiceState::AwaitingResponse;
  snapshot_.chunksSent = stats.chunksSent;
  snapshot_.samplesSent = stats.samplesSent;
  snapshot_.bytesSent = stats.bytesSent;
  perfTracer_.listenStop();
}

}  // namespace tongdou
