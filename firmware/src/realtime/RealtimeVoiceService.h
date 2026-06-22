#pragma once

#include <Arduino.h>

#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "realtime/RealtimeAudioSender.h"

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
  String lastErrorCode;
  String lastErrorMessage;
};

class RealtimeVoiceService {
 public:
  RealtimeVoiceService(AudioInput& audioInput, AudioOutput& audioOutput);

  void begin();
  void update();
  bool startTurn(uint16_t captureMs = 800);
  bool playResponse(uint16_t clipId, const String& text);
  void failTurn(const String& code, const String& message);
  bool consumeFailureEvent();
  RealtimeVoiceSnapshot snapshot() const;
  const char* stateName() const;

 private:
  void captureFrame(unsigned long nowMs);
  void finishCapture();

  AudioInput& audioInput_;
  AudioOutput& audioOutput_;
  RealtimeAudioSender sender_;
  RealtimeVoiceSnapshot snapshot_;
  unsigned long captureUntilMs_ = 0;
  unsigned long nextCaptureMs_ = 0;
  bool failurePending_ = false;
};

}  // namespace tongdou
