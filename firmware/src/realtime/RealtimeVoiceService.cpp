#include "realtime/RealtimeVoiceService.h"

#include <cstring>

#include <WiFi.h>

namespace tongdou {
namespace {

constexpr uint16_t kMinCaptureMs = 200;
constexpr uint16_t kMaxCaptureMs = 8000;
constexpr unsigned long kFrameIntervalMs = 5;
constexpr unsigned long kAwaitingResponseTimeoutMs = 12000;
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

RealtimeVoiceService::RealtimeVoiceService(LocalAudioPipeline& audioPipeline,
                                           AudioOutput& audioOutput)
    : audioPipeline_(audioPipeline), audioOutput_(audioOutput) {}

void RealtimeVoiceService::begin() {
  sender_.begin();
  snapshot_ = {};
  snapshot_.state = RealtimeVoiceState::Idle;
  snapshot_.audioInputReady = audioPipeline_.ready();
  snapshot_.audioOutputReady = audioOutput_.ready();
}

void RealtimeVoiceService::update() {
  if (backendStarted_) {
    voiceClient_.update();
    updateConversationExit();
    voiceClient_.playNextDownlinkPacket(audioOutput_);
    updateBackendConnectionFailure();
  }

  snapshot_.audioInputReady = audioPipeline_.ready();
  snapshot_.audioOutputReady = audioOutput_.ready();
  completeAutoCaptureWhenBackendResponds();
  updateBackendTurnCompletion();
  updateConversationExit();

  if (snapshot_.state != RealtimeVoiceState::Listening) {
    return;
  }

  const unsigned long now = millis();
  if (captureDeadlineEnabled_ &&
      static_cast<long>(now - captureUntilMs_) >= 0) {
    finishCapture();
    return;
  }
  if (static_cast<long>(now - nextCaptureMs_) < 0) {
    return;
  }

  sendAvailableAudioFrames(now);
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
    snapshot_.lastErrorCode = "voice_backend_not_configured";
    snapshot_.lastErrorMessage = "voice backend is not configured";
    return false;
  }

  TongDouVoiceHelloConfig hello;
  hello.audio.sampleRate = 16000;
  hello.audio.channels = 1;
  hello.audio.frameDurationMs = 60;
  hello.mcpEnabled = false;

  backendStarted_ = voiceClient_.begin(backendConfig_, hello);
  if (!backendStarted_) {
    const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
    snapshot_.lastErrorCode = backend.lastError.length() > 0
                                  ? backend.lastError
                                  : String("voice_client_begin_failed");
    snapshot_.lastErrorMessage = "voice client failed to initialize";
    return false;
  }

  snapshot_.lastErrorCode = "";
  snapshot_.lastErrorMessage = "";
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
  audioPipeline_.stopUpload();
  capturedPcmFrameUsed_ = 0;
  const bool sent = voiceClient_.sendAbort();
  if (sent) {
    snapshot_.state = RealtimeVoiceState::Idle;
  }
  return sent;
}

bool RealtimeVoiceService::startTurn(uint16_t captureMs) {
  return startTurnWithMode(TongDouVoiceListenMode::Manual, captureMs,
                           TurnKind::Regular);
}

bool RealtimeVoiceService::startAutoTurn() {
  return startTurnWithMode(TongDouVoiceListenMode::Auto, 0, TurnKind::Regular);
}

bool RealtimeVoiceService::startConversationTurn() {
  return startTurnWithMode(TongDouVoiceListenMode::Auto, 0,
                           TurnKind::ConversationFollowUp);
}

bool RealtimeVoiceService::startTurnWithMode(TongDouVoiceListenMode mode,
                                             uint16_t captureMs,
                                             TurnKind turnKind) {
  if (snapshot_.state == RealtimeVoiceState::Listening ||
      snapshot_.state == RealtimeVoiceState::Ending ||
      voiceClient_.conversationExitPending()) {
    failTurn("voice_busy", "voice turn is already listening");
    return false;
  }
  if (!audioPipeline_.ready()) {
    failTurn("audio_input_not_ready", "microphone is not ready");
    return false;
  }

  sender_.reset();
  audioPipeline_.clearUploadQueue();
  capturedPcmFrameUsed_ = 0;
  turnStartedMs_ = millis();
  firstPcmFrameLogged_ = false;
  firstOpusPacketLogged_ = false;
  currentTurnKind_ = turnKind;
  goodbyeLatched_ = false;
  exitSoftwareDrainLogged_ = false;

  if (backendStarted_) {
    const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
    backendDownlinkPacketsAtTurnStart_ = backend.downlinkPacketsPlayed;
    backendBinaryPacketsAtTurnStart_ = backend.binaryPacketsReceived;
    if (voiceClient_.state() == TongDouVoiceClientState::Speaking ||
        voiceClient_.state() == TongDouVoiceClientState::PreparingSpeak) {
      abortBackendSpeech();
    }
    if (!voiceClient_.ready()) {
      failTurn("voice_backend_not_ready", "voice backend is not ready");
      return false;
    }
    if (!voiceClient_.sendListenStart(mode)) {
      failTurn("voice_listen_start_failed", "failed to start backend listening");
      return false;
    }
    Serial.print(F("voice timeline listen_start_sent t="));
    Serial.print(millis());
    Serial.print(F(" mode="));
    Serial.print(tongDouVoiceListenModeName(mode));
    Serial.print(F(" session="));
    Serial.println(voiceClient_.snapshot().sessionId);
  }

  currentListenMode_ = mode;
  ++snapshot_.turnId;
  snapshot_.state = RealtimeVoiceState::Listening;
  snapshot_.lastTurnOk = false;
  snapshot_.conversationExitRequested = false;
  snapshot_.backendGoodbyeReceived = false;
  snapshot_.backendGoodbyeReason = "";
  snapshot_.lastErrorCode = "";
  snapshot_.lastErrorMessage = "";
  turnCompletedPending_ = false;
  conversationTimeoutPending_ = false;
  conversationEndedPending_ = false;
  snapshot_.chunksSent = 0;
  snapshot_.samplesSent = 0;
  snapshot_.bytesSent = 0;

  const unsigned long now = millis();
  captureDeadlineEnabled_ = mode != TongDouVoiceListenMode::Auto;
  const uint16_t safeCaptureMs = clampCaptureMs(captureMs);
  captureUntilMs_ = captureDeadlineEnabled_ ? now + safeCaptureMs : 0;
  nextCaptureMs_ = now;
  Serial.print(F("voice timeline turn_start t="));
  Serial.print(now);
  Serial.print(F(" mode="));
  Serial.println(tongDouVoiceListenModeName(mode));
  audioPipeline_.startUpload();
  if (!backendStarted_) {
    perfTracer_.listenStart();
  }
  return true;
}

bool RealtimeVoiceService::requestConversationExit() {
  if (!backendStarted_ || !voiceClient_.snapshot().connected) {
    failTurn("voice_exit_backend_unavailable",
             "voice backend is unavailable during manual exit");
    return false;
  }

  audioPipeline_.stopUpload();
  capturedPcmFrameUsed_ = 0;
  if (!voiceClient_.requestConversationExit("manual")) {
    failTurn("voice_exit_request_failed",
             "failed to request conversation exit from backend");
    return false;
  }

  snapshot_.state = RealtimeVoiceState::Ending;
  snapshot_.conversationExitRequested = true;
  turnCompletedPending_ = false;
  conversationTimeoutPending_ = false;
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
  audioPipeline_.stopUpload();
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

bool RealtimeVoiceService::consumeTurnCompletedEvent() {
  if (!turnCompletedPending_) {
    return false;
  }

  turnCompletedPending_ = false;
  return true;
}

bool RealtimeVoiceService::consumeConversationTimeoutEvent() {
  if (!conversationTimeoutPending_) {
    return false;
  }

  conversationTimeoutPending_ = false;
  return true;
}

bool RealtimeVoiceService::consumeConversationEndedEvent() {
  if (!conversationEndedPending_) {
    return false;
  }

  conversationEndedPending_ = false;
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
    const TongDouVoiceAudioCodecStats stats = codec.stats();
    result.error = stats.lastError;
    result.nativeError = stats.lastNativeError;
    result.nativeErrorText = stats.lastNativeErrorText;
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
  copy.audioInputReady = audioPipeline_.ready();
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
  copy.backendCodecNativeError = backend.codecStats.lastNativeError;
  copy.backendCodecNativeErrorText = backend.codecStats.lastNativeErrorText;
  copy.backendSessionId = backend.sessionId;
  copy.backendLastMessageType = tongDouVoiceMessageTypeName(backend.lastMessageType);
  copy.backendLastTtsState = tongDouVoiceTtsStateName(backend.lastTtsState);
  copy.backendLastText = backend.lastText;
  copy.backendLastEmotion = backend.lastEmotion;
  copy.backendCodecLastError = backend.codecStats.lastError;
  copy.backendLastError = backend.lastError;
  copy.conversationExitRequested = backend.conversationExitRequested;
  copy.backendGoodbyeReceived = backend.goodbyeReceived;
  copy.backendGoodbyeReason = backend.goodbyeReason;
  return copy;
}

const char* RealtimeVoiceService::stateName() const {
  switch (snapshot_.state) {
    case RealtimeVoiceState::Listening:
      return "listening";
    case RealtimeVoiceState::AwaitingResponse:
      return "awaiting_response";
    case RealtimeVoiceState::Ending:
      return "ending";
    case RealtimeVoiceState::Failed:
      return "failed";
    case RealtimeVoiceState::Idle:
    default:
      return "idle";
  }
}

void RealtimeVoiceService::sendAvailableAudioFrames(unsigned long nowMs) {
  while (audioPipeline_.readUploadFrame(capturedPcmFrame_,
                                        kCapturedPcmFrameSamples)) {
    capturedPcmFrameUsed_ = kCapturedPcmFrameSamples;
    logFirstPcmFrame(capturedPcmFrame_, capturedPcmFrameUsed_, nowMs);
    if (!sendCapturedFrame(false)) {
      return;
    }
  }

  nextCaptureMs_ = nowMs + kFrameIntervalMs;
}

bool RealtimeVoiceService::sendCapturedFrame(bool padWithSilence) {
  if (capturedPcmFrameUsed_ == 0) {
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

  if (backendStarted_) {
    if (!voiceClient_.sendPcm16Frame(capturedPcmFrame_, kCapturedPcmFrameSamples)) {
      failTurn("voice_audio_send_failed", "failed to encode or send voice audio");
      capturedPcmFrameUsed_ = 0;
      return false;
    }

    if (!firstOpusPacketLogged_) {
      firstOpusPacketLogged_ = true;
      const unsigned long now = millis();
      Serial.print(F("voice timeline first_opus_sent t="));
      Serial.print(now);
      Serial.print(F(" dt="));
      Serial.print(now - turnStartedMs_);
      Serial.print(F(" samples="));
      Serial.println(kCapturedPcmFrameSamples);
    }
  }

  sender_.sendPcm16(capturedPcmFrame_, kCapturedPcmFrameSamples);
  if (!backendStarted_ && sender_.stats().chunksSent == 1) {
    perfTracer_.firstAudioSent();
  }
  capturedPcmFrameUsed_ = 0;
  return true;
}

void RealtimeVoiceService::finishCapture() {
  if (backendStarted_ && capturedPcmFrameUsed_ == 0) {
    capturedPcmFrameUsed_ =
        audioPipeline_.readUploadSamples(capturedPcmFrame_, kCapturedPcmFrameSamples);
  }

  audioPipeline_.stopUpload();
  if (backendStarted_ && capturedPcmFrameUsed_ > 0 && !sendCapturedFrame(true)) {
    return;
  }

  const RealtimeAudioSendStats stats = sender_.stats();
  if (stats.samplesSent == 0) {
    failTurn("audio_capture_empty", "microphone returned no samples");
    return;
  }

  if (backendStarted_ && !voiceClient_.sendListenStop(currentListenMode_)) {
    failTurn("voice_listen_stop_failed", "failed to stop backend listening");
    return;
  }

  Serial.print(F("voice timeline listen_stop_sent t="));
  Serial.print(millis());
  Serial.print(F(" dt="));
  Serial.print(millis() - turnStartedMs_);
  Serial.print(F(" chunks="));
  Serial.print(stats.chunksSent);
  Serial.print(F(" samples="));
  Serial.println(stats.samplesSent);

  snapshot_.state = RealtimeVoiceState::AwaitingResponse;
  snapshot_.chunksSent = stats.chunksSent;
  snapshot_.samplesSent = stats.samplesSent;
  snapshot_.bytesSent = stats.bytesSent;
  awaitingResponseStartedMs_ = millis();
  if (!backendStarted_) {
    perfTracer_.listenStop();
  }
}

void RealtimeVoiceService::logFirstPcmFrame(const int16_t* samples,
                                            size_t sampleCount,
                                            unsigned long nowMs) {
  if (firstPcmFrameLogged_ || samples == nullptr || sampleCount == 0) {
    return;
  }

  firstPcmFrameLogged_ = true;
  int64_t absSum = 0;
  int32_t peak = 0;
  for (size_t i = 0; i < sampleCount; ++i) {
    const int32_t sample = samples[i];
    const int32_t magnitude = sample < 0 ? -sample : sample;
    absSum += magnitude;
    if (magnitude > peak) {
      peak = magnitude;
    }
  }
  const int32_t meanAbs =
      static_cast<int32_t>(absSum / static_cast<int64_t>(sampleCount));
  Serial.print(F("voice timeline first_pcm t="));
  Serial.print(nowMs);
  Serial.print(F(" dt="));
  Serial.print(nowMs - turnStartedMs_);
  Serial.print(F(" samples="));
  Serial.print(sampleCount);
  Serial.print(F(" mean_abs="));
  Serial.print(meanAbs);
  Serial.print(F(" peak="));
  Serial.println(peak);
}

void RealtimeVoiceService::completeAutoCaptureWhenBackendResponds() {
  if (!backendStarted_ || snapshot_.state != RealtimeVoiceState::Listening ||
      currentListenMode_ != TongDouVoiceListenMode::Auto) {
    return;
  }

  const TongDouVoiceClientState backendState = voiceClient_.state();
  const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
  const bool backendIsResponding =
      backendState == TongDouVoiceClientState::PreparingSpeak ||
      backendState == TongDouVoiceClientState::Speaking ||
      (backendState == TongDouVoiceClientState::Ready &&
       backend.binaryPacketsReceived > backendBinaryPacketsAtTurnStart_);
  if (!backendIsResponding) {
    return;
  }

  audioPipeline_.stopUpload();
  capturedPcmFrameUsed_ = 0;

  const RealtimeAudioSendStats stats = sender_.stats();
  snapshot_.state = RealtimeVoiceState::AwaitingResponse;
  snapshot_.chunksSent = stats.chunksSent;
  snapshot_.samplesSent = stats.samplesSent;
  snapshot_.bytesSent = stats.bytesSent;
  awaitingResponseStartedMs_ = millis();

  Serial.println(F("voice auto backend response detected"));
}

void RealtimeVoiceService::updateBackendTurnCompletion() {
  if (!backendStarted_ || snapshot_.state != RealtimeVoiceState::AwaitingResponse) {
    return;
  }

  const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
  if (backend.binaryPacketsReceived <= backendBinaryPacketsAtTurnStart_ &&
      static_cast<long>(millis() - awaitingResponseStartedMs_) >=
          static_cast<long>(kAwaitingResponseTimeoutMs)) {
    if (currentTurnKind_ == TurnKind::ConversationFollowUp) {
      snapshot_.state = RealtimeVoiceState::Idle;
      snapshot_.lastTurnOk = true;
      snapshot_.lastErrorCode = "";
      snapshot_.lastErrorMessage = "";
      currentTurnKind_ = TurnKind::Regular;
      conversationTimeoutPending_ = true;
      Serial.println(F("conversation follow-up timeout"));
      return;
    }
    failTurn("voice_response_timeout", "voice backend returned no response");
    return;
  }

  if (voiceClient_.state() != TongDouVoiceClientState::Ready ||
      backend.queueStats.downlinkQueued > 0 ||
      backend.downlinkPacketsPlayed <= backendDownlinkPacketsAtTurnStart_) {
    return;
  }

  snapshot_.state = RealtimeVoiceState::Idle;
  snapshot_.lastTurnOk = true;
  snapshot_.lastErrorCode = "";
  snapshot_.lastErrorMessage = "";
  turnCompletedPending_ = true;
  currentTurnKind_ = TurnKind::Regular;
}

void RealtimeVoiceService::updateConversationExit() {
  if (!backendStarted_) {
    return;
  }

  const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
  if (backend.goodbyeReceived && !goodbyeLatched_) {
    goodbyeLatched_ = true;
    snapshot_.state = RealtimeVoiceState::Ending;
    snapshot_.conversationExitRequested = true;
    audioPipeline_.stopUpload();
    capturedPcmFrameUsed_ = 0;
    turnCompletedPending_ = false;
    conversationTimeoutPending_ = false;
    Serial.print(F("voice conversation ending reason="));
    Serial.println(backend.goodbyeReason);
  }

  if (snapshot_.state != RealtimeVoiceState::Ending || !goodbyeLatched_ ||
      !backend.ttsStopped || !voiceClient_.downlinkDrained()) {
    return;
  }

  if (!exitSoftwareDrainLogged_) {
    exitSoftwareDrainLogged_ = true;
    Serial.println(F("voice goodbye software queue drained"));
  }
  if (!audioOutput_.streamDrained()) {
    return;
  }

  Serial.println(F("voice goodbye speaker drained state=idle"));
  snapshot_.state = RealtimeVoiceState::Idle;
  snapshot_.lastTurnOk = true;
  snapshot_.lastErrorCode = "";
  snapshot_.lastErrorMessage = "";
  conversationEndedPending_ = true;
  currentTurnKind_ = TurnKind::Regular;
}

void RealtimeVoiceService::updateBackendConnectionFailure() {
  const TongDouVoiceClientSnapshot backend = voiceClient_.snapshot();
  const bool turnActive = snapshot_.state == RealtimeVoiceState::Listening ||
                          snapshot_.state == RealtimeVoiceState::AwaitingResponse ||
                          snapshot_.state == RealtimeVoiceState::Ending;
  if (backendWasConnected_ && !backend.connected && turnActive &&
      !goodbyeLatched_ && !backend.goodbyeReceived) {
    audioOutput_.stopStream();
    failTurn("voice_backend_disconnected",
             "voice backend disconnected during an active conversation");
    Serial.println(F("voice abnormal fallback reason=backend_disconnected"));
  }
  backendWasConnected_ = backend.connected;
}

}  // namespace tongdou
