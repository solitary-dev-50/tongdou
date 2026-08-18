#include "tongdou_voice/TongDouVoiceClient.h"

namespace tongdou {

void TongDouVoiceClient::begin(const TongDouVoiceConnectionConfig& connectionConfig,
                               const TongDouVoiceHelloConfig& helloConfig) {
  helloConfig_ = helloConfig;
  snapshot_ = {};
  queues_.clear();
  snapshot_.downstreamAudio = helloConfig_.audio;
  audioCodec_.begin(helloConfig_.audio);
  snapshot_.codecStats = audioCodec_.stats();
  if (connectionConfig.host.length() == 0) {
    snapshot_.lastError = "voice_server_host_missing";
    setState(TongDouVoiceClientState::Failed);
    return;
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
}

void TongDouVoiceClient::update() {
  socket_.update();
  snapshot_.connected = socket_.connected();
  snapshot_.codecStats = audioCodec_.stats();
}

void TongDouVoiceClient::disconnect() {
  socket_.disconnect();
  queues_.clear();
  snapshot_.connected = false;
  snapshot_.ready = false;
  setState(TongDouVoiceClientState::Offline);
}

bool TongDouVoiceClient::sendListenStart() {
  if (!ready()) {
    return false;
  }
  if (!sendText(protocol_.buildListenStart(snapshot_.sessionId))) {
    return false;
  }
  perfTracer_.listenStart();
  setState(TongDouVoiceClientState::Listening);
  return true;
}

bool TongDouVoiceClient::sendListenStop() {
  if (snapshot_.state != TongDouVoiceClientState::Listening) {
    return false;
  }
  if (!sendText(protocol_.buildListenStop(snapshot_.sessionId))) {
    return false;
  }
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
  setState(TongDouVoiceClientState::Speaking);
  return true;
}

TongDouVoiceClientSnapshot TongDouVoiceClient::snapshot() const {
  TongDouVoiceClientSnapshot copy = snapshot_;
  copy.queueStats = queues_.stats();
  copy.codecStats = audioCodec_.stats();
  return copy;
}

TongDouVoiceClientState TongDouVoiceClient::state() const {
  return snapshot_.state;
}

bool TongDouVoiceClient::ready() const {
  return snapshot_.ready && socket_.connected();
}

void TongDouVoiceClient::handleSocketState(TongDouVoiceWebSocketState state) {
  snapshot_.connected = state == TongDouVoiceWebSocketState::Connected;
  if (state == TongDouVoiceWebSocketState::Connected) {
    setState(TongDouVoiceClientState::Handshaking);
    if (!sendText(protocol_.buildHello(helloConfig_))) {
      snapshot_.lastError = "hello_send_failed";
      setState(TongDouVoiceClientState::Failed);
    }
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
    snapshot_.ready = true;
    setState(TongDouVoiceClientState::Ready);
    return;
  }

  if (message.type == TongDouVoiceMessageType::Tts) {
    if (message.ttsState == TongDouVoiceTtsState::Start ||
        message.ttsState == TongDouVoiceTtsState::SentenceStart) {
      setState(TongDouVoiceClientState::PreparingSpeak);
    } else if (message.ttsState == TongDouVoiceTtsState::Stop) {
      perfTracer_.ttsDone();
      setState(TongDouVoiceClientState::Ready);
    }
  }
}

void TongDouVoiceClient::handleBinaryMessage(const uint8_t* payload, size_t length) {
  if (length == 0) {
    return;
  }
  ++snapshot_.binaryPacketsReceived;
  queues_.pushDownlink(payload, length);
  perfTracer_.firstTtsPacketReceived();
  setState(TongDouVoiceClientState::Speaking);
}

void TongDouVoiceClient::setState(TongDouVoiceClientState state) {
  snapshot_.state = state;
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
    case TongDouVoiceClientState::Failed:
      return "failed";
    case TongDouVoiceClientState::Offline:
    default:
      return "offline";
  }
}

}  // namespace tongdou
