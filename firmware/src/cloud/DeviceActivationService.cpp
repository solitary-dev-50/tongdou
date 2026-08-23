#include "cloud/DeviceActivationService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "tongdou/BuildInfo.h"
#include "tongdou/ServerConfig.h"

namespace tongdou {
namespace {

constexpr const char* kBoardType = "tongdou-esp32s3";
constexpr const char* kOtaUrl = TONGDOU_OTA_URL;

String macAddress() {
  return WiFi.macAddress();
}

String localIpAddress() {
  return WiFi.localIP().toString();
}

String urlEncode(const String& value) {
  String encoded;
  const char* hex = "0123456789ABCDEF";
  for (unsigned int i = 0; i < value.length(); ++i) {
    const char c = value[i];
    const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                      c == '.' || c == '~';
    if (safe) {
      encoded += c;
    } else {
      encoded += '%';
      encoded += hex[(static_cast<uint8_t>(c) >> 4) & 0x0F];
      encoded += hex[static_cast<uint8_t>(c) & 0x0F];
    }
  }
  return encoded;
}

}  // namespace

DeviceActivationService::DeviceActivationService(
    RealtimeVoiceService& realtimeVoiceService, FaceDisplay& faceDisplay)
    : realtimeVoiceService_(realtimeVoiceService), faceDisplay_(faceDisplay) {}

void DeviceActivationService::begin() {
  state_ = State::WaitingForWifi;
  scheduleRetry(kInitialDelayMs);
  Serial.print("device activation ota=");
  Serial.println(kOtaUrl);
}

void DeviceActivationService::update() {
  if (state_ == State::Activated) {
    return;
  }
  if (!WiFi.isConnected()) {
    return;
  }

  const unsigned long now = millis();
  if (static_cast<long>(now - nextAttemptMs_) < 0) {
    return;
  }

  requestActivation();
}

bool DeviceActivationService::activated() const {
  return state_ == State::Activated;
}

String DeviceActivationService::activationCode() const {
  return activationCode_;
}

String DeviceActivationService::lastError() const {
  return lastError_;
}

bool DeviceActivationService::consumeActivationCodeEvent(String& code) {
  if (!activationCodeEventPending_) {
    return false;
  }

  code = activationCodeEvent_;
  activationCodeEventPending_ = false;
  return true;
}

void DeviceActivationService::requestActivation() {
  HTTPClient http;
  if (!http.begin(kOtaUrl)) {
    lastError_ = "ota_begin_failed";
    Serial.println("device activation failed: ota_begin_failed");
    scheduleRetry(kRetryIntervalMs);
    return;
  }

  const String deviceId = macAddress();
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Device-Id", deviceId);
  http.addHeader("Client-Id", deviceId);

  const String body = buildReportJson();
  const int status = http.POST(body);
  if (status <= 0) {
    lastError_ = "ota_post_failed";
    Serial.print("device activation failed: ota_post_failed status=");
    Serial.println(status);
    http.end();
    scheduleRetry(kRetryIntervalMs);
    return;
  }

  const String response = http.getString();
  http.end();

  if (status < 200 || status >= 300) {
    lastError_ = "ota_http_error";
    Serial.print("device activation failed: http status=");
    Serial.println(status);
    scheduleRetry(kRetryIntervalMs);
    return;
  }

  if (!applyResponse(response)) {
    scheduleRetry(kRetryIntervalMs);
    return;
  }

  if (state_ != State::Activated) {
    scheduleRetry(kRetryIntervalMs);
  }
}

String DeviceActivationService::buildReportJson() const {
  JsonDocument doc;
  const String deviceId = macAddress();

  doc["version"] = 1;
  doc["flash_size"] = ESP.getFlashChipSize();
  doc["minimum_free_heap_size"] = ESP.getMinFreeHeap();
  doc["mac_address"] = deviceId;
  doc["uuid"] = deviceId;
  doc["chip_model_name"] = ESP.getChipModel();
  doc["chip_info"]["model"] = 0;
  doc["chip_info"]["cores"] = ESP.getChipCores();
  doc["chip_info"]["revision"] = ESP.getChipRevision();
  doc["chip_info"]["features"] = 0;
  doc["application"]["name"] = kProjectName;
  doc["application"]["version"] = kFirmwareStage;
  doc["application"]["compile_time"] = String(__DATE__) + " " + String(__TIME__);
  doc["application"]["idf_version"] = ESP.getSdkVersion();
  doc["application"]["elf_sha256"] = "";
  doc["ota"]["label"] = "app";
  doc["board"]["type"] = kBoardType;
  doc["board"]["ssid"] = WiFi.SSID();
  doc["board"]["rssi"] = WiFi.RSSI();
  doc["board"]["channel"] = WiFi.channel();
  doc["board"]["ip"] = localIpAddress();
  doc["board"]["mac"] = deviceId;

  String output;
  serializeJson(doc, output);
  return output;
}

bool DeviceActivationService::applyResponse(const String& response) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, response);
  if (error) {
    lastError_ = "ota_json_parse_failed";
    Serial.print("device activation failed: json parse ");
    Serial.println(error.c_str());
    return false;
  }

  const char* responseError = doc["error"] | "";
  if (strlen(responseError) > 0) {
    lastError_ = responseError;
    Serial.print("device activation failed: ");
    Serial.println(responseError);
    return false;
  }

  const char* code = doc["activation"]["code"] | "";
  if (strlen(code) > 0) {
    const String newCode(code);
    state_ = State::WaitingForBinding;
    if (activationCode_ != newCode || !codeDisplayed_) {
      activationCode_ = newCode;
      codeDisplayed_ = true;
      activationCodeEvent_ = activationCode_;
      activationCodeEventPending_ = true;
      Serial.print("device activation code=");
      Serial.println(activationCode_);
      faceDisplay_.showActivationCode(activationCode_);
    }
    return true;
  }

  const char* websocketUrl = doc["websocket"]["url"] | "";
  const char* token = doc["websocket"]["token"] | "";
  if (strlen(websocketUrl) == 0) {
    lastError_ = "websocket_url_missing";
    Serial.println("device activation failed: websocket_url_missing");
    return false;
  }

  if (!voiceConfigured_ && !configureVoiceBackend(String(websocketUrl), String(token))) {
    return false;
  }

  state_ = State::Activated;
  activationCode_ = "";
  activationCodeEvent_ = "";
  activationCodeEventPending_ = false;
  Serial.println("device activation active");
  return true;
}

bool DeviceActivationService::configureVoiceBackend(const String& websocketUrl,
                                                    const String& token) {
  TongDouVoiceConnectionConfig config;
  if (!parseWebsocketUrl(websocketUrl, config)) {
    lastError_ = "websocket_url_invalid";
    Serial.print("device activation failed: websocket_url_invalid ");
    Serial.println(websocketUrl);
    return false;
  }

  config.authorization = token;
  config.deviceId = macAddress();
  config.clientId = macAddress();
  const char separator = config.path.indexOf('?') >= 0 ? '&' : '?';
  config.path += separator;
  config.path += "device-id=";
  config.path += urlEncode(config.deviceId);
  config.path += "&client-id=";
  config.path += urlEncode(config.clientId);
  if (token.length() > 0) {
    config.path += "&authorization=";
    config.path += urlEncode(String("Bearer ") + token);
  }

  realtimeVoiceService_.configureBackend(config);
  if (!realtimeVoiceService_.connectBackend()) {
    lastError_ = "voice_connect_start_failed";
    Serial.println("device activation failed: voice_connect_start_failed");
    return false;
  }

  voiceConfigured_ = true;
  Serial.print("voice backend configured host=");
  Serial.print(config.host);
  Serial.print(" port=");
  Serial.print(config.port);
  Serial.print(" path=");
  const int queryIndex = config.path.indexOf('?');
  Serial.println(queryIndex >= 0 ? config.path.substring(0, queryIndex) : config.path);
  return true;
}

bool DeviceActivationService::parseWebsocketUrl(
    const String& websocketUrl, TongDouVoiceConnectionConfig& config) const {
  String url = websocketUrl;
  url.trim();

  if (url.startsWith("wss://")) {
    config.useTls = true;
    url.remove(0, 6);
    config.port = 443;
  } else if (url.startsWith("ws://")) {
    config.useTls = false;
    url.remove(0, 5);
    config.port = 80;
  } else {
    return false;
  }

  const int pathIndex = url.indexOf('/');
  String hostPort = pathIndex >= 0 ? url.substring(0, pathIndex) : url;
  config.path = pathIndex >= 0 ? url.substring(pathIndex) : "/";

  const int colonIndex = hostPort.lastIndexOf(':');
  if (colonIndex > 0) {
    config.host = hostPort.substring(0, colonIndex);
    config.port = static_cast<uint16_t>(hostPort.substring(colonIndex + 1).toInt());
  } else {
    config.host = hostPort;
  }

  return config.host.length() > 0 && config.port > 0 && config.path.length() > 0;
}

void DeviceActivationService::scheduleRetry(unsigned long delayMs) {
  nextAttemptMs_ = millis() + delayMs;
}

}  // namespace tongdou
