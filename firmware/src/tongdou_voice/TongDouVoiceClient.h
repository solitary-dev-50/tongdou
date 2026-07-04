#pragma once

#include <Arduino.h>

#include "hardware/AudioOutput.h"
#include "tongdou_voice/TongDouVoiceConfig.h"
#include "tongdou_voice/TongDouVoiceAudioCodec.h"
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
  uint32_t uplinkPcmFramesEncoded = 0;
  uint32_t uplinkOpusPacketsSent = 0;
  uint32_t uplinkEncodeFailures = 0;
  uint32_t downlinkPacketsPlayed = 0;
  uint32_t downlinkPlaybackFailures = 0;
  TongDouVoiceQueueStats queueStats;
  TongDouVoiceMessageType lastMessageType = TongDouVoiceMessageType::Unknown;
  TongDouVoiceTtsState lastTtsState = TongDouVoiceTtsState::None;
  String lastText;
  String lastEmotion;
  String lastError;
  TongDouVoiceAudioParams downstreamAudio;
  TongDouVoiceAudioCodecStats codecStats;
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
  bool sendPcm16Frame(const int16_t* samples, size_t sampleCount);
  bool sendAudioPacket(const uint8_t* payload, size_t length);
  bool playNextDownlinkPacket(AudioOutput& audioOutput);

  TongDouVoiceClientSnapshot snapshot() const;
  TongDouVoiceClientState state() const;
  bool ready() const;

 private:
  void handleSocketState(TongDouVoiceWebSocketState state);
  void handleTextMessage(const String& payload);
  void handleBinaryMessage(const uint8_t* payload, size_t length);
  void setState(TongDouVoiceClientState state);
  bool sendText(const String& payload);

  static constexpr size_t kMaxDecodedSamples = 2880;

  TongDouVoiceProtocol protocol_;
  TongDouVoiceAudioCodec audioCodec_;
  TongDouVoiceWebSocketClient socket_;
  TongDouVoiceStreamQueues queues_;
  uint8_t uplinkPacket_[TongDouVoiceStreamQueues::kMaxPacketBytes] = {};
  uint8_t downlinkPacket_[TongDouVoiceStreamQueues::kMaxPacketBytes] = {};
  int16_t decodedPcm_[kMaxDecodedSamples] = {};
  TongDouVoiceHelloConfig helloConfig_;
  TongDouVoiceClientSnapshot snapshot_;
  TongDouVoicePerfTracer perfTracer_;
};

const char* tongDouVoiceClientStateName(TongDouVoiceClientState state);

}  // namespace tongdou
