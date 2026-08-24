#include "tongdou_voice/TongDouVoiceClient.h"

namespace tongdou {
namespace {

void printVoiceMemorySnapshot(const char* phase) {
  const uint32_t psramTotal = ESP.getPsramSize();
  Serial.print(F("voice memory phase="));
  Serial.print(phase);
  Serial.print(F(" psram_found="));
  Serial.print(psramTotal > 0 ? 1 : 0);
  Serial.print(F(" psram_total="));
  Serial.print(psramTotal);
  Serial.print(F(" psram_free="));
  Serial.print(ESP.getFreePsram());
  Serial.print(F(" psram_largest="));
  Serial.print(ESP.getMaxAllocPsram());
  Serial.print(F(" internal_free="));
  Serial.print(ESP.getFreeHeap());
  Serial.print(F(" internal_min_free="));
  Serial.print(ESP.getMinFreeHeap());
  Serial.print(F(" internal_largest="));
  Serial.println(ESP.getMaxAllocHeap());
}

}  // namespace

bool TongDouVoiceClient::begin(const TongDouVoiceConnectionConfig& connectionConfig,
                               const TongDouVoiceHelloConfig& helloConfig) {
  helloConfig_ = helloConfig;
  snapshot_ = {};
  queues_.clear();
  snapshot_.downstreamAudio = helloConfig_.audio;
  printVoiceMemorySnapshot("before_codec");
  const bool codecStarted = audioCodec_.begin(helloConfig_.audio);
  printVoiceMemorySnapshot("after_codec");
  snapshot_.codecStats = audioCodec_.stats();
  if (!codecStarted) {
    snapshot_.lastError = snapshot_.codecStats.lastError;
    Serial.print(F("voice codec unavailable error="));
    Serial.print(snapshot_.codecStats.lastError);
    Serial.print(F(" native_error="));
    Serial.print(snapshot_.codecStats.lastNativeError);
    Serial.print(F(" native_text="));
    Serial.println(snapshot_.codecStats.lastNativeErrorText);
    setState(TongDouVoiceClientState::Failed);
    return false;
  }
  if (connectionConfig.host.length() == 0) {
    snapshot_.lastError = "voice_server_host_missing";
    setState(TongDouVoiceClientState::Failed);
    return false;
  }

  socket_.onStateChanged([this](TongDouVoiceWebSocketState state) {
    handleSocketState(state);
  });
  socket_.onText([this](const String& payload) {
    handleTextMessage(payload);
  });
  socket_.onBinary([this](const uint8_t* payload, size_t length) {
    handleBinaryMessage(payload, length);
  });

  setState(TongDouVoiceClientState::Connecting);
  socket_.begin(connectionConfig);
  return true;
}

void TongDouVoiceClient::update() {
  socket_.update();
  snapshot_.connected = socket_.connected();
  snapshot_.codecStats = audioCodec_.stats();

  if (snapshot_.state == TongDouVoiceClientState::Handshaking &&
      socket_.connected() &&
      static_cast<long>(millis() - nextHelloRetryMs_) >= 0) {
    sendHello();
  }
}

void TongDouVoiceClient::disconnect() {
  socket_.disconnect();
  queues_.clear();
  snapshot_.connected = false;
  snapshot_.ready = false;
  setState(TongDouVoiceClientState::Offline);
}

void TongDouVoiceClient::onMcpPayload(McpPayloadHandler callback) {
  mcpPayloadHandler_ = callback;
}

bool TongDouVoiceClient::sendListenStart(TongDouVoiceListenMode mode) {
  if (!ready() || conversationExitPending()) {
    return false;
  }
  if (!sendText(protocol_.buildListenStart(snapshot_.sessionId, mode))) {
    return false;
  }
  Serial.print(F("voice listen start mode="));
  Serial.println(tongDouVoiceListenModeName(mode));
  perfTracer_.listenStart();
  setState(TongDouVoiceClientState::Listening);
  return true;
}

bool TongDouVoiceClient::sendListenStop(TongDouVoiceListenMode mode) {
  if (snapshot_.state != TongDouVoiceClientState::Listening) {
    return false;
  }
  if (!sendText(protocol_.buildListenStop(snapshot_.sessionId, mode))) {
    return false;
  }
  Serial.print(F("voice listen stop mode="));
  Serial.println(tongDouVoiceListenModeName(mode));
  perfTracer_.listenStop();
  setState(TongDouVoiceClientState::Ready);
  return true;
}

bool TongDouVoiceClient::sendListenDetect(const String& text) {
  if (!ready() || text.length() == 0) {
    return false;
  }
  return sendText(protocol_.buildListenDetect(snapshot_.sessionId, text));
}

bool TongDouVoiceClient::sendAbort() {
  if (!socket_.connected()) {
    return false;
  }
  const bool sent = sendText(protocol_.buildAbort(snapshot_.sessionId));
  if (sent) {
    queues_.clear();
    setState(TongDouVoiceClientState::Ready);
  }
  return sent;
}

bool TongDouVoiceClient::requestConversationExit(const String& reason) {
  if (!socket_.connected() || snapshot_.goodbyeReceived) {
    return false;
  }
  if (!snapshot_.conversationExitRequested &&
      !sendText(protocol_.buildGoodbyeRequest(snapshot_.sessionId, reason))) {
    return false;
  }

  snapshot_.conversationExitRequested = true;
  snapshot_.ready = false;
  snapshot_.goodbyeReason = reason;
  setState(TongDouVoiceClientState::Ending);
  Serial.print(F("voice conversation exit requested reason="));
  Serial.println(reason);
  return true;
}

bool TongDouVoiceClient::sendPcm16Frame(const int16_t* samples, size_t sampleCount) {
  size_t opusLength = 0;
  if (!audioCodec_.encodePcm16ToOpus(samples, sampleCount, uplinkPacket_,
                                     sizeof(uplinkPacket_), opusLength)) {
    ++snapshot_.uplinkEncodeFailures;
    snapshot_.lastError = audioCodec_.stats().lastError;
    snapshot_.codecStats = audioCodec_.stats();
    return false;
  }

  ++snapshot_.uplinkPcmFramesEncoded;
  snapshot_.codecStats = audioCodec_.stats();
  perfTracer_.firstOpusEncoded();

  if (!sendAudioPacket(uplinkPacket_, opusLength)) {
    return false;
  }

  ++snapshot_.uplinkOpusPacketsSent;
  return true;
}

bool TongDouVoiceClient::sendAudioPacket(const uint8_t* payload, size_t length) {
  if (snapshot_.state != TongDouVoiceClientState::Listening) {
    return false;
  }
  const bool sent = socket_.sendBinary(payload, length);
  if (sent) {
    perfTracer_.firstAudioSent();
  }
  return sent;
}

bool TongDouVoiceClient::playNextDownlinkPacket(AudioOutput& audioOutput) {
  size_t packetLength = 0;
  if (!queues_.popDownlink(downlinkPacket_, sizeof(downlinkPacket_), packetLength)) {
    return false;
  }
  if (!audioOutput.ready()) {
    ++snapshot_.downlinkPlaybackFailures;
    snapshot_.lastError = "audio_output_not_ready";
    return false;
  }

  size_t decodedSamples = 0;
  if (!audioCodec_.decodeOpusToPcm16(downlinkPacket_, packetLength, decodedPcm_,
                                     kMaxDecodedSamples, decodedSamples)) {
    ++snapshot_.downlinkPlaybackFailures;
    snapshot_.lastError = audioCodec_.stats().lastError;
    snapshot_.codecStats = audioCodec_.stats();
    return false;
  }

  if (!audioOutput.writePcm16Mono(decodedPcm_, decodedSamples,
                                  snapshot_.downstreamAudio.sampleRate)) {
    ++snapshot_.downlinkPlaybackFailures;
    snapshot_.lastError = "audio_output_write_failed";
    return false;
  }

  ++snapshot_.downlinkPacketsPlayed;
  perfTracer_.speakerStart();
  setState(conversationExitPending() ? TongDouVoiceClientState::Ending
                                     : TongDouVoiceClientState::Speaking);
  return true;
}

TongDouVoiceClientSnapshot TongDouVoiceClient::snapshot() const {
  TongDouVoiceClientSnapshot copy = snapshot_;
  copy.ready = ready();
  copy.queueStats = queues_.stats();
  copy.codecStats = audioCodec_.stats();
  return copy;
}

TongDouVoiceClientState TongDouVoiceClient::state() const {
  return snapshot_.state;
}

bool TongDouVoiceClient::ready() const {
  return snapshot_.ready && socket_.connected() && audioCodec_.ready() &&
         !conversationExitPending();
}

bool TongDouVoiceClient::conversationExitPending() const {
  return snapshot_.conversationExitRequested || snapshot_.goodbyeReceived;
}

bool TongDouVoiceClient::downlinkDrained() const {
  return queues_.stats().downlinkQueued == 0;
}

void TongDouVoiceClient::handleSocketState(TongDouVoiceWebSocketState state) {
  snapshot_.connected = state == TongDouVoiceWebSocketState::Connected;
  if (state == TongDouVoiceWebSocketState::Connected) {
    setState(TongDouVoiceClientState::Handshaking);
    nextHelloRetryMs_ = 0;
    sendHello();
    return;
  }
  if (state == TongDouVoiceWebSocketState::Disconnected) {
    snapshot_.ready = false;
    setState(TongDouVoiceClientState::Offline);
  }
}

void TongDouVoiceClient::handleTextMessage(const String& payload) {
  ++snapshot_.textMessagesReceived;

  TongDouVoiceIncomingMessage message;
  String error;
  if (!protocol_.parseTextMessage(payload, message, error)) {
    snapshot_.lastError = error;
    setState(TongDouVoiceClientState::Failed);
    return;
  }

  snapshot_.lastMessageType = message.type;
  snapshot_.lastTtsState = message.ttsState;
  snapshot_.lastText = message.text;
  snapshot_.lastEmotion = message.emotion;
  if (message.sessionId.length() > 0) {
    snapshot_.sessionId = message.sessionId;
  }
  if (message.hasAudioParams) {
    snapshot_.downstreamAudio = message.audio;
  }

  if (message.type == TongDouVoiceMessageType::Hello) {
    if (!audioCodec_.ready()) {
      snapshot_.ready = false;
      snapshot_.lastError = "voice_codec_not_ready";
      setState(TongDouVoiceClientState::Failed);
      return;
    }
    Serial.print("voice session=");
    Serial.println(snapshot_.sessionId);
    snapshot_.conversationExitRequested = false;
    snapshot_.goodbyeReceived = false;
    snapshot_.goodbyeReason = "";
    snapshot_.ttsStopped = true;
    snapshot_.ready = true;
    setState(TongDouVoiceClientState::Ready);
    return;
  }

  if (message.type == TongDouVoiceMessageType::Tts) {
    if (message.ttsState == TongDouVoiceTtsState::Start ||
        message.ttsState == TongDouVoiceTtsState::SentenceStart) {
      snapshot_.ttsStopped = false;
      setState(TongDouVoiceClientState::PreparingSpeak);
    } else if (message.ttsState == TongDouVoiceTtsState::Stop) {
      snapshot_.ttsStopped = true;
      perfTracer_.ttsDone();
      setState(conversationExitPending() ? TongDouVoiceClientState::Ending
                                         : TongDouVoiceClientState::Ready);
    }
    return;
  }

  if (message.type == TongDouVoiceMessageType::Mcp) {
    ++snapshot_.mcpRequestsReceived;
    if (!mcpPayloadHandler_) {
      ++snapshot_.mcpFailures;
      snapshot_.lastError = "mcp_handler_missing";
      Serial.println(F("voice mcp failed reason=handler_missing"));
      return;
    }

    const String responsePayload = mcpPayloadHandler_(message.mcpPayload);
    const String responseMessage = protocol_.buildMcpMessage(responsePayload);
    if (responseMessage.length() == 0 || !sendText(responseMessage)) {
      ++snapshot_.mcpFailures;
      snapshot_.lastError = "mcp_response_send_failed";
      Serial.println(F("voice mcp failed reason=response_send_failed"));
      return;
    }

    ++snapshot_.mcpResponsesSent;
    Serial.print(F("voice mcp response sent requests="));
    Serial.print(snapshot_.mcpRequestsReceived);
    Serial.print(F(" responses="));
    Serial.println(snapshot_.mcpResponsesSent);
    return;
  }

  if (message.type == TongDouVoiceMessageType::Goodbye) {
    snapshot_.conversationExitRequested = true;
    snapshot_.goodbyeReceived = true;
    snapshot_.ready = false;
    snapshot_.goodbyeReason = message.reason;
    setState(TongDouVoiceClientState::Ending);
    Serial.print(F("voice goodbye received session="));
    Serial.print(snapshot_.sessionId);
    Serial.print(F(" reason="));
    Serial.println(snapshot_.goodbyeReason);
  }
}

void TongDouVoiceClient::handleBinaryMessage(const uint8_t* payload, size_t length) {
  if (length == 0) {
    return;
  }
  ++snapshot_.binaryPacketsReceived;
  queues_.pushDownlink(payload, length);
  perfTracer_.firstTtsPacketReceived();
  setState(conversationExitPending() ? TongDouVoiceClientState::Ending
                                     : TongDouVoiceClientState::Speaking);
}

void TongDouVoiceClient::setState(TongDouVoiceClientState state) {
  if (state == TongDouVoiceClientState::Offline ||
      state == TongDouVoiceClientState::Failed) {
    snapshot_.ready = false;
  }
  if (snapshot_.state == state) {
    return;
  }
  snapshot_.state = state;
  Serial.print("voice client state=");
  Serial.println(tongDouVoiceClientStateName(state));
}

bool TongDouVoiceClient::sendHello() {
  if (!sendText(protocol_.buildHello(helloConfig_))) {
    snapshot_.lastError = "hello_send_failed";
    setState(TongDouVoiceClientState::Failed);
    return false;
  }

  nextHelloRetryMs_ = millis() + kHelloRetryIntervalMs;
  Serial.println("voice hello sent");
  return true;
}

bool TongDouVoiceClient::sendText(const String& payload) {
  if (!socket_.sendText(payload)) {
    snapshot_.lastError = "websocket_send_failed";
    return false;
  }
  return true;
}

const char* tongDouVoiceClientStateName(TongDouVoiceClientState state) {
  switch (state) {
    case TongDouVoiceClientState::Connecting:
      return "connecting";
    case TongDouVoiceClientState::Handshaking:
      return "handshaking";
    case TongDouVoiceClientState::Ready:
      return "ready";
    case TongDouVoiceClientState::Listening:
      return "listening";
    case TongDouVoiceClientState::PreparingSpeak:
      return "preparing_speak";
    case TongDouVoiceClientState::Speaking:
      return "speaking";
    case TongDouVoiceClientState::Ending:
      return "ending";
    case TongDouVoiceClientState::Failed:
      return "failed";
    case TongDouVoiceClientState::Offline:
    default:
      return "offline";
  }
}

}  // namespace tongdou
