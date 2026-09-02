#pragma once

#include <Arduino.h>

#include "hardware/FaceDisplay.h"
#include "realtime/RealtimeVoiceService.h"
#include "system/ServerEndpointStore.h"

namespace tongdou {

class DeviceActivationService {
 public:
  DeviceActivationService(RealtimeVoiceService& realtimeVoiceService,
                          FaceDisplay& faceDisplay,
                          ServerEndpointStore& serverEndpointStore);

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
  static constexpr unsigned long kFirstFailureRetryMs = 1000;
  static constexpr unsigned long kSecondFailureRetryMs = 2000;
  static constexpr unsigned long kThirdFailureRetryMs = 5000;
  static constexpr unsigned long kSteadyRetryIntervalMs = 30000;

  void requestActivation();
  String buildReportJson() const;
  bool applyResponse(const String& response);
  bool configureVoiceBackend(const String& websocketUrl, const String& token);
  bool parseWebsocketUrl(const String& websocketUrl, TongDouVoiceConnectionConfig& config) const;
  void scheduleFailureRetry();
  void scheduleRetry(unsigned long delayMs);

  RealtimeVoiceService& realtimeVoiceService_;
  FaceDisplay& faceDisplay_;
  ServerEndpointStore& serverEndpointStore_;
  State state_ = State::WaitingForWifi;
  unsigned long nextAttemptMs_ = 0;
  uint8_t consecutiveFailures_ = 0;
  String activationCode_;
  String activationCodeEvent_;
  String lastError_;
  bool codeDisplayed_ = false;
  bool activationCodeEventPending_ = false;
  bool voiceConfigured_ = false;
};

}  // namespace tongdou
