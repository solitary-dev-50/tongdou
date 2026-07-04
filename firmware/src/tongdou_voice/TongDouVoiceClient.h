#pragma once

#include <Arduino.h>

#include "tongdou_voice/TongDouVoiceConfig.h"
#include "tongdou_voice/TongDouVoicePerfTracer.h"
#include "tongdou_voice/TongDouVoiceProtocol.h"
#include "tongdou_voice/TongDouVoiceStreamQueues.h"
#include "tongdou_voice/TongDouVoiceWebSocketClient.h"

namespace tongdou {

enum class TongDouVoiceClientState : uint8_t {
  Offline,
  Connecting,
  Handshaking,
  Ready,
  Listening,
  PreparingSpeak,
  Speaking,
  Failed,
};

struct TongDouVoiceClientSnapshot {
  TongDouVoiceClientState state = TongDouVoiceClientState::Offline;
  String sessionId;
  bool connected = false;
  bool ready = false;
  uint32_t textMessagesReceived = 0;
  uint32_t binaryPacketsReceived = 0;
  TongDouVoiceQueueStats queueStats;
  TongDouVoiceMessageType lastMessageType = TongDouVoiceMessageType::Unknown;
  TongDouVoiceTtsState lastTtsState = TongDouVoiceTtsState::None;
  String lastText;
  String lastEmotion;
  String lastError;
  TongDouVoiceAudioParams downstreamAudio;
};

class TongDouVoiceClient {
 public:
  void begin(const TongDouVoiceConnectionConfig& connectionConfig,
             const TongDouVoiceHelloConfig& helloConfig = TongDouVoiceHelloConfig{});
  void update();
  void disconnect();

  bool sendListenStart();
  bool sendListenStop();
  bool sendListenDetect(const String& text);
  bool sendAbort();
  bool sendAudioPacket(const uint8_t* payload, size_t length);

  TongDouVoiceClientSnapshot snapshot() const;
  TongDouVoiceClientState state() const;
  bool ready() const;

 private:
  void handleSocketState(TongDouVoiceWebSocketState state);
  void handleTextMessage(const String& payload);
  void handleBinaryMessage(const uint8_t* payload, size_t length);
  void setState(TongDouVoiceClientState state);
  bool sendText(const String& payload);

  TongDouVoiceProtocol protocol_;
  TongDouVoiceWebSocketClient socket_;
  TongDouVoiceStreamQueues queues_;
  TongDouVoiceHelloConfig helloConfig_;
  TongDouVoiceClientSnapshot snapshot_;
  TongDouVoicePerfTracer perfTracer_;
};

const char* tongDouVoiceClientStateName(TongDouVoiceClientState state);

}  // namespace tongdou
