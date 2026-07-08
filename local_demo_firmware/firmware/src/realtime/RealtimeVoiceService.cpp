#include "realtime/RealtimeVoiceService.h"

#include <cstring>

#include <WiFi.h>

namespace tongdou {
namespace {

constexpr uint16_t kMinCaptureMs = 200;
constexpr uint16_t kMaxCaptureMs = 4000;
constexpr unsigned long kFrameIntervalMs = 8;
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
  if (backendStarted_) {
    voiceClient_.update();
    voiceClient_.playNextDownlinkPacket(audioOutput_);
  }

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

void RealtimeVoiceService::configureBackend(const TongDouVoiceConnectionConfig& config) {
  backendConfig_ = config;
  if (backendConfig_.deviceId.length() == 0) {
    backendConfig_.deviceId = WiFi.macAddress();
  }
  backendConfigured_ = backendConfig_.host.length() > 0;
  backendStarted_ = false;
  voiceClient_.disconnect();
}

bool RealtimeVoiceService::connectBackend() {
  if (!backendConfigured_) {
    return false;
  }

  TongDouVoiceHelloConfig hello;
  hello.audio.sampleRate = 16000;
  hello.audio.channels = 1;
  hello.audio.frameDurationMs = 60;
  hello.mcpEnabled = false;

  voiceClient_.begin(backendConfig_, hello);
  backendStarted_ = true;
  return true;
}

bool RealtimeVoiceService::sendDetectText(const String& text) {
  if (!backendStarted_ || text.length() == 0) {
    return false;
  }
  return voiceClient_.sendListenDetect(text);
}

bool RealtimeVoiceService::abortBackendSpeech() {
  if (!backendStarted_) {
    return false;
  }

  audioOutput_.stopStream();
  capturedPcmFrameUsed_ = 0;
  const bool sent = voiceClient_.sendAbort();
  if (sent) {
    snapshot_.state = RealtimeVoiceState::Idle;
  }
  return sent;
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
  capturedPcmFrameUsed_ = 0;

  if (backendStarted_) {
    if (voiceClient_.state() == TongDouVoiceClientState::Speaking ||
        voiceClient_.state() == TongDouVoiceClientState::PreparingSpeak) {
      abortBackendSpeech();
    }
    if (!voiceClient_.ready()) {
      failTurn("voice_backend_not_ready", "voice backend is not ready");
      return false;
    }
    if (!voiceClient_.sendListenStart()) {
      failTurn("voice_listen_start_failed", "failed to start backend listening");
      return false;
    }
  }

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
  if (!backendStarted_) {
    perfTracer_.listenStart();
  }
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

TongDouVoiceAudioCodecSelfTestResult RealtimeVoiceService::runCodecSelfTest() {
  TongDouVoiceAudioCodec codec;
  TongDouVoiceAudioParams audio;
  audio.format = "opus";
  audio.sampleRate = 16000;
  audio.channels = 1;
  audio.frameDurationMs = 60;

  if (!codec.begin(audio)) {
    TongDouVoiceAudioCodecSelfTestResult result;
    result.error = codec.stats().lastError;
    return result;
  }

  return codec.runSelfTest();
}

RealtimeVoiceSnapshot RealtimeVoiceService::snapshot() const {
  RealtimeVoiceSnapshot copy = snapshot_;
  const RealtimeAudioSendStats stats = sender_.stats();
  const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
  copy.chunksSent = stats.chunksSent;
  copy.samplesSent = stats.samplesSent;
  copy.bytesSent = stats.bytesSent;
  copy.audioInputReady = audioInput_.ready();
  copy.audioOutputReady = audioOutput_.ready();
  copy.backendState = tongDouVoiceClientStateName(backend.state);
  copy.backendConnected = backend.connected;
  copy.backendReady = backend.ready;
  copy.backendTextMessagesReceived = backend.textMessagesReceived;
  copy.backendBinaryPacketsReceived = backend.binaryPacketsReceived;
  copy.backendUplinkPcmFramesEncoded = backend.uplinkPcmFramesEncoded;
  copy.backendUplinkOpusPacketsSent = backend.uplinkOpusPacketsSent;
  copy.backendUplinkEncodeFailures = backend.uplinkEncodeFailures;
  copy.backendDownlinkPacketsPlayed = backend.downlinkPacketsPlayed;
  copy.backendDownlinkPlaybackFailures = backend.downlinkPlaybackFailures;
  copy.backendDownlinkQueued = backend.queueStats.downlinkQueued;
  copy.backendDownlinkPushed = backend.queueStats.downlinkPushed;
  copy.backendDownlinkPopped = backend.queueStats.downlinkPopped;
  copy.backendDownlinkDropped = backend.queueStats.downlinkDropped;
  copy.backendDownlinkOversized = backend.queueStats.downlinkOversized;
  copy.backendCodecReady = backend.codecStats.ready;
  copy.backendCodecEncodeAttempts = backend.codecStats.encodeAttempts;
  copy.backendCodecDecodeAttempts = backend.codecStats.decodeAttempts;
  copy.backendCodecEncodeFailures = backend.codecStats.encodeFailures;
  copy.backendCodecDecodeFailures = backend.codecStats.decodeFailures;
  copy.backendSessionId = backend.sessionId;
  copy.backendLastMessageType = tongDouVoiceMessageTypeName(backend.lastMessageType);
  copy.backendLastTtsState = tongDouVoiceTtsStateName(backend.lastTtsState);
  copy.backendLastText = backend.lastText;
  copy.backendLastEmotion = backend.lastEmotion;
  copy.backendCodecLastError = backend.codecStats.lastError;
  copy.backendLastError = backend.lastError;
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
    if (!backendStarted_ && sender_.sendPcm16(samples, samplesRead)) {
      perfTracer_.firstAudioSent();
    }
    if (backendStarted_) {
      size_t offset = 0;
      while (offset < samplesRead) {
        const size_t available = kCapturedPcmFrameSamples - capturedPcmFrameUsed_;
        const size_t toCopy = min(available, samplesRead - offset);
        memcpy(capturedPcmFrame_ + capturedPcmFrameUsed_, samples + offset,
               toCopy * sizeof(samples[0]));
        capturedPcmFrameUsed_ += toCopy;
        offset += toCopy;

        if (capturedPcmFrameUsed_ >= kCapturedPcmFrameSamples) {
          sendCapturedFrame(false);
        }
      }
    }
  }

  nextCaptureMs_ = nowMs + kFrameIntervalMs;
}

bool RealtimeVoiceService::sendCapturedFrame(bool padWithSilence) {
  if (!backendStarted_ || capturedPcmFrameUsed_ == 0) {
    return false;
  }

  if (capturedPcmFrameUsed_ < kCapturedPcmFrameSamples) {
    if (!padWithSilence) {
      return false;
    }
    memset(capturedPcmFrame_ + capturedPcmFrameUsed_, 0,
           (kCapturedPcmFrameSamples - capturedPcmFrameUsed_) *
               sizeof(capturedPcmFrame_[0]));
  }

  if (!voiceClient_.sendPcm16Frame(capturedPcmFrame_, kCapturedPcmFrameSamples)) {
    failTurn("voice_audio_send_failed", "failed to encode or send voice audio");
    capturedPcmFrameUsed_ = 0;
    return false;
  }

  sender_.sendPcm16(capturedPcmFrame_, kCapturedPcmFrameSamples);
  capturedPcmFrameUsed_ = 0;
  return true;
}

void RealtimeVoiceService::finishCapture() {
  if (backendStarted_ && capturedPcmFrameUsed_ > 0 &&
      !sendCapturedFrame(true)) {
    return;
  }

  const RealtimeAudioSendStats stats = sender_.stats();
  if (stats.samplesSent == 0) {
    failTurn("audio_capture_empty", "microphone returned no samples");
    return;
  }

  if (backendStarted_ && !voiceClient_.sendListenStop()) {
    failTurn("voice_listen_stop_failed", "failed to stop backend listening");
    return;
  }

  snapshot_.state = RealtimeVoiceState::AwaitingResponse;
  snapshot_.chunksSent = stats.chunksSent;
  snapshot_.samplesSent = stats.samplesSent;
  snapshot_.bytesSent = stats.bytesSent;
  if (!backendStarted_) {
    perfTracer_.listenStop();
  }
}

}  // namespace tongdou
