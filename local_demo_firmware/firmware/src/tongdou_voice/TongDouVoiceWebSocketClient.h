#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>

#include <functional>

#include "tongdou_voice/TongDouVoiceConfig.h"

namespace tongdou {

enum class TongDouVoiceWebSocketState : uint8_t {
  Disconnected,
  Connecting,
  Connected,
};

class TongDouVoiceWebSocketClient {
 public:
  using TextCallback = std::function<void(const String&)>;
  using BinaryCallback = std::function<void(const uint8_t*, size_t)>;
  using StateCallback = std::function<void(TongDouVoiceWebSocketState)>;

  void begin(const TongDouVoiceConnectionConfig& config);
  void update();
  void disconnect();

  bool sendText(const String& payload);
  bool sendBinary(const uint8_t* payload, size_t length);

  void onText(TextCallback callback);
  void onBinary(BinaryCallback callback);
  void onStateChanged(StateCallback callback);

  TongDouVoiceWebSocketState state() const;
  bool connected() const;

 private:
  void handleEvent(WStype_t type, uint8_t* payload, size_t length);
  void setState(TongDouVoiceWebSocketState state);
  void rebuildHeaders();

  WebSocketsClient socket_;
  TongDouVoiceConnectionConfig config_;
  TongDouVoiceWebSocketState state_ = TongDouVoiceWebSocketState::Disconnected;
  String headers_;
  TextCallback textCallback_;
  BinaryCallback binaryCallback_;
  StateCallback stateCallback_;
};

const char* tongDouVoiceWebSocketStateName(TongDouVoiceWebSocketState state);

}  // namespace tongdou
