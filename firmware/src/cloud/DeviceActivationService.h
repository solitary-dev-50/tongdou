#pragma once

#include <Arduino.h>

#include "hardware/FaceDisplay.h"
#include "realtime/RealtimeVoiceService.h"

namespace tongdou {

class DeviceActivationService {
 public:
  DeviceActivationService(RealtimeVoiceService& realtimeVoiceService,
                          FaceDisplay& faceDisplay);

  void begin();
  void update();
  bool activated() const;
  String activationCode() const;
  String lastError() const;
  bool consumeActivationCodeEvent(String& code);

 private:
  enum class State : uint8_t {
    WaitingForWifi,
    WaitingForBinding,
    Activated,
  };

  static constexpr unsigned long kInitialDelayMs = 3000;
  static constexpr unsigned long kRetryIntervalMs = 30000;

  void requestActivation();
  String buildReportJson() const;
  bool applyResponse(const String& response);
  bool configureVoiceBackend(const String& websocketUrl, const String& token);
  bool parseWebsocketUrl(const String& websocketUrl, TongDouVoiceConnectionConfig& config) const;
  void scheduleRetry(unsigned long delayMs);

  RealtimeVoiceService& realtimeVoiceService_;
  FaceDisplay& faceDisplay_;
  State state_ = State::WaitingForWifi;
  unsigned long nextAttemptMs_ = 0;
  String activationCode_;
  String activationCodeEvent_;
  String lastError_;
  bool codeDisplayed_ = false;
  bool activationCodeEventPending_ = false;
  bool voiceConfigured_ = false;
};

}  // namespace tongdou
