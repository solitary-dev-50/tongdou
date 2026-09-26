#pragma once

#include <Arduino.h>

namespace tongdou {

struct TongDouVoiceAudioParams {
  String format = "opus";
  uint16_t sampleRate = 16000;
  uint8_t channels = 1;
  uint16_t frameDurationMs = 60;
};

struct TongDouVoiceHelloConfig {
  uint8_t version = 1;
  String transport = "websocket";
  bool mcpEnabled = false;
  bool goodbyeEnabled = true;
  TongDouVoiceAudioParams audio;
};

struct TongDouVoiceConnectionConfig {
  String host;
  uint16_t port = 8100;
  String path = "/xiaozhi/v1/";
  bool useTls = false;
  const char* rootCa = nullptr;
  String authorization;
  String deviceId;
  String clientId = "tongdou-demo";
  uint8_t protocolVersion = 1;
};

}  // namespace tongdou
