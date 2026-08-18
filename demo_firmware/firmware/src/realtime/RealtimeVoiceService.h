#pragma once

#include <Arduino.h>

#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "realtime/RealtimeAudioSender.h"
#include "tongdou_voice/TongDouVoiceAudioCodec.h"
#include "tongdou_voice/TongDouVoiceClient.h"
#include "tongdou_voice/TongDouVoicePerfTracer.h"

namespace tongdou {

enum class RealtimeVoiceState : uint8_t {
  Idle,
  Listening,
  AwaitingResponse,
  Failed,
};

struct RealtimeVoiceSnapshot {
  RealtimeVoiceState state = RealtimeVoiceState::Idle;
  uint32_t turnId = 0;
  bool audioInputReady = false;
  bool audioOutputReady = false;
  bool lastTurnOk = false;
  uint32_t chunksSent = 0;
  uint32_t samplesSent = 0;
  uint32_t bytesSent = 0;
  String backendState;
  bool backendConnected = false;
  bool backendReady = false;
  uint32_t backendTextMessagesReceived = 0;
  uint32_t backendBinaryPacketsReceived = 0;
  uint32_t backendUplinkPcmFramesEncoded = 0;
  uint32_t backendUplinkOpusPacketsSent = 0;
  uint32_t backendUplinkEncodeFailures = 0;
  uint32_t backendDownlinkPacketsPlayed = 0;
  uint32_t backendDownlinkPlaybackFailures = 0;
  uint16_t backendDownlinkQueued = 0;
  uint32_t backendDownlinkPushed = 0;
  uint32_t backendDownlinkPopped = 0;
  uint32_t backendDownlinkDropped = 0;
  uint32_t backendDownlinkOversized = 0;
  bool backendCodecReady = false;
  uint32_t backendCodecEncodeAttempts = 0;
  uint32_t backendCodecDecodeAttempts = 0;
  uint32_t backendCodecEncodeFailures = 0;
  uint32_t backendCodecDecodeFailures = 0;
  String backendSessionId;
  String backendLastMessageType;
  String backendLastTtsState;
  String backendLastText;
  String backendLastEmotion;
  String backendCodecLastError;
  String backendLastError;
  String lastErrorCode;
  String lastErrorMessage;
};

class RealtimeVoiceService {
 public:
  RealtimeVoiceService(AudioInput& audioInput, AudioOutput& audioOutput);

  void begin();
  void update();
  void configureBackend(const TongDouVoiceConnectionConfig& config);
  bool connectBackend();
  bool sendDetectText(const String& text);
  bool abortBackendSpeech();
  bool startTurn(uint16_t captureMs = 2500);
  bool playResponse(uint16_t clipId, const String& text);
  void failTurn(const String& code, const String& message);
  bool consumeFailureEvent();
  TongDouVoiceAudioCodecSelfTestResult runCodecSelfTest();
  RealtimeVoiceSnapshot snapshot() const;
  const char* stateName() const;

 private:
  static constexpr size_t kCapturedPcmFrameSamples = 960;

  void captureFrame(unsigned long nowMs);
  bool sendCapturedFrame(bool padWithSilence);
  void finishCapture();

  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  RealtimeAudioSender sender_;
  TongDouVoicePerfTracer perfTracer_;
  TongDouVoiceClient voiceClient_;
  TongDouVoiceConnectionConfig backendConfig_;
  bool backendConfigured_ = false;
  bool backendStarted_ = false;
  RealtimeVoiceSnapshot snapshot_;
  unsigned long captureUntilMs_ = 0;
  unsigned long nextCaptureMs_ = 0;
  int16_t capturedPcmFrame_[kCapturedPcmFrameSamples] = {};
  size_t capturedPcmFrameUsed_ = 0;
  bool failurePending_ = false;
};

}  // namespace tongdou
