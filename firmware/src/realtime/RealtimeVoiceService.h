#pragma once

#include <Arduino.h>

#include "hardware/AudioOutput.h"
#include "realtime/LocalAudioPipeline.h"
#include "realtime/RealtimeAudioSender.h"
#include "tongdou_voice/TongDouVoiceAudioCodec.h"
#include "tongdou_voice/TongDouVoiceClient.h"
#include "tongdou_voice/TongDouVoicePerfTracer.h"

namespace tongdou {

enum class RealtimeVoiceState : uint8_t {
  Idle,
  Listening,
  AwaitingResponse,
  Ending,
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
  uint32_t backendMcpRequestsReceived = 0;
  uint32_t backendMcpResponsesSent = 0;
  uint32_t backendMcpFailures = 0;
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
  int32_t backendCodecNativeError = 0;
  String backendCodecNativeErrorText;
  String backendSessionId;
  String backendLocaleResourceBaseUrl;
  uint16_t backendLocaleResourceFormatVersion = 0;
  String backendLastMessageType;
  String backendLastTtsState;
  String backendLastText;
  String backendLastEmotion;
  String backendCodecLastError;
  String backendLastError;
  bool conversationExitRequested = false;
  bool backendGoodbyeReceived = false;
  String backendGoodbyeReason;
  String lastErrorCode;
  String lastErrorMessage;
};

class RealtimeVoiceService {
 public:
  RealtimeVoiceService(LocalAudioPipeline& audioPipeline, AudioOutput& audioOutput);

  void begin();
  void update();
  void setMcpPayloadHandler(TongDouVoiceClient::McpPayloadHandler handler);
  void configureBackend(const TongDouVoiceConnectionConfig& config);
  bool connectBackend();
  bool sendDetectText(const String& text);
  bool startWakeGreeting(const String& text);
  bool abortBackendSpeech();
  bool requestConversationExit();
  bool startTurn(uint16_t captureMs = 2500);
  bool startAutoTurn();
  bool startConversationTurn();
  bool playResponse(uint16_t clipId, const String& text);
  void failTurn(const String& code, const String& message);
  bool consumeFailureEvent();
  bool consumeTurnCompletedEvent();
  bool consumeConversationTimeoutEvent();
  bool consumeConversationEndedEvent();
  TongDouVoiceAudioCodecSelfTestResult runCodecSelfTest();
  RealtimeVoiceSnapshot snapshot() const;
  const char* stateName() const;

 private:
  enum class TurnKind : uint8_t {
    Regular,
    ConversationFollowUp,
  };

  static constexpr size_t kCapturedPcmFrameSamples =
      LocalAudioPipeline::kUploadFrameSamples;

  bool startTurnWithMode(TongDouVoiceListenMode mode, uint16_t captureMs,
                         TurnKind turnKind);
  void sendAvailableAudioFrames(unsigned long nowMs);
  bool sendCapturedFrame(bool padWithSilence);
  void finishCapture();
  void completeAutoCaptureWhenBackendResponds();
  void updateBackendTurnCompletion();
  void updateConversationExit();
  void updateBackendConnectionFailure();
  void logFirstPcmFrame(const int16_t* samples, size_t sampleCount,
                        unsigned long nowMs);

  LocalAudioPipeline& audioPipeline_;
  AudioOutput& audioOutput_;
  RealtimeAudioSender sender_;
  TongDouVoicePerfTracer perfTracer_;
  TongDouVoiceClient voiceClient_;
  TongDouVoiceConnectionConfig backendConfig_;
  bool backendConfigured_ = false;
  bool backendStarted_ = false;
  bool mcpEnabled_ = false;
  RealtimeVoiceSnapshot snapshot_;
  TongDouVoiceListenMode currentListenMode_ = TongDouVoiceListenMode::Manual;
  unsigned long captureUntilMs_ = 0;
  bool captureDeadlineEnabled_ = false;
  unsigned long nextCaptureMs_ = 0;
  unsigned long awaitingResponseStartedMs_ = 0;
  int16_t capturedPcmFrame_[kCapturedPcmFrameSamples] = {};
  size_t capturedPcmFrameUsed_ = 0;
  uint32_t backendDownlinkPacketsAtTurnStart_ = 0;
  uint32_t backendBinaryPacketsAtTurnStart_ = 0;
  unsigned long turnStartedMs_ = 0;
  bool firstPcmFrameLogged_ = false;
  bool firstOpusPacketLogged_ = false;
  bool failurePending_ = false;
  bool turnCompletedPending_ = false;
  bool conversationTimeoutPending_ = false;
  bool conversationEndedPending_ = false;
  bool goodbyeLatched_ = false;
  bool exitSoftwareDrainLogged_ = false;
  bool backendWasConnected_ = false;
  TurnKind currentTurnKind_ = TurnKind::Regular;
};

}  // namespace tongdou
