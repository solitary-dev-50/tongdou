#include "tongdou_voice/TongDouVoiceWebSocketClient.h"

namespace tongdou {
namespace {

constexpr unsigned long kReconnectIntervalMs = 5000;
constexpr unsigned long kPingIntervalMs = 15000;
constexpr unsigned long kPongTimeoutMs = 3000;
constexpr uint8_t kDisconnectAfterMissedPongs = 2;

}  // namespace

void TongDouVoiceWebSocketClient::begin(const TongDouVoiceConnectionConfig& config) {
  config_ = config;
  rebuildHeaders();
  setState(TongDouVoiceWebSocketState::Connecting);

  socket_.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
    handleEvent(type, payload, length);
  });
  socket_.setReconnectInterval(kReconnectIntervalMs);
  socket_.enableHeartbeat(kPingIntervalMs, kPongTimeoutMs, kDisconnectAfterMissedPongs);
  if (headers_.length() > 0) {
    socket_.setExtraHeaders(headers_.c_str());
  }

  if (config_.useTls) {
    socket_.beginSSL(config_.host.c_str(), config_.port, config_.path.c_str());
  } else {
    socket_.begin(config_.host.c_str(), config_.port, config_.path.c_str());
  }
}

void TongDouVoiceWebSocketClient::update() {
  socket_.loop();
}

void TongDouVoiceWebSocketClient::disconnect() {
  socket_.disconnect();
  setState(TongDouVoiceWebSocketState::Disconnected);
}

bool TongDouVoiceWebSocketClient::sendText(const String& payload) {
  if (!connected()) {
    return false;
  }
  String copy = payload;
  return socket_.sendTXT(copy);
}

bool TongDouVoiceWebSocketClient::sendBinary(const uint8_t* payload, size_t length) {
  if (!connected() || payload == nullptr || length == 0) {
    return false;
  }
  return socket_.sendBIN(const_cast<uint8_t*>(payload), length);
}

void TongDouVoiceWebSocketClient::onText(TextCallback callback) {
  textCallback_ = callback;
}

void TongDouVoiceWebSocketClient::onBinary(BinaryCallback callback) {
  binaryCallback_ = callback;
}

void TongDouVoiceWebSocketClient::onStateChanged(StateCallback callback) {
  stateCallback_ = callback;
}

TongDouVoiceWebSocketState TongDouVoiceWebSocketClient::state() const {
  return state_;
}

bool TongDouVoiceWebSocketClient::connected() const {
  return state_ == TongDouVoiceWebSocketState::Connected;
}

void TongDouVoiceWebSocketClient::handleEvent(WStype_t type, uint8_t* payload,
                                              size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      setState(TongDouVoiceWebSocketState::Connected);
      break;
    case WStype_DISCONNECTED:
      setState(TongDouVoiceWebSocketState::Disconnected);
      break;
    case WStype_TEXT:
      if (textCallback_) {
        textCallback_(String(reinterpret_cast<const char*>(payload)));
      }
      break;
    case WStype_BIN:
      if (binaryCallback_) {
        binaryCallback_(payload, length);
      }
      break;
    default:
      break;
  }
}

void TongDouVoiceWebSocketClient::setState(TongDouVoiceWebSocketState state) {
  if (state_ == state) {
    return;
  }

  state_ = state;
  if (stateCallback_) {
    stateCallback_(state_);
  }
}

void TongDouVoiceWebSocketClient::rebuildHeaders() {
  headers_ = "";
  if (config_.authorization.length() > 0) {
    headers_ += "Authorization: ";
    if (!config_.authorization.startsWith("Bearer ")) {
      headers_ += "Bearer ";
    }
    headers_ += config_.authorization;
    headers_ += "\r\n";
  }
  headers_ += "Protocol-Version: ";
  headers_ += config_.protocolVersion;
  headers_ += "\r\n";
  if (config_.deviceId.length() > 0) {
    headers_ += "Device-Id: ";
    headers_ += config_.deviceId;
    headers_ += "\r\n";
  }
  if (config_.clientId.length() > 0) {
    headers_ += "Client-Id: ";
    headers_ += config_.clientId;
    headers_ += "\r\n";
  }
}

const char* tongDouVoiceWebSocketStateName(TongDouVoiceWebSocketState state) {
  switch (state) {
    case TongDouVoiceWebSocketState::Connecting:
      return "connecting";
    case TongDouVoiceWebSocketState::Connected:
      return "connected";
    case TongDouVoiceWebSocketState::Disconnected:
    default:
      return "disconnected";
  }
}

}  // namespace tongdou
