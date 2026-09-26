#include "cloud/DeviceActivationService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

#if TONGDOU_XIAOZHI_CLOUD
#include <WiFiClientSecure.h>
#include "tongdou/XiaozhiCloudRootCa.h"
#endif

#include "tongdou/BuildInfo.h"
#include "tongdou/BoardProfile.h"
#include "tongdou/ServerConfig.h"

namespace tongdou {
namespace {

constexpr const char* kOtaUrl = TONGDOU_OTA_URL;

String macAddress() {
  String address = WiFi.macAddress();
#if TONGDOU_XIAOZHI_CLOUD
  address.toLowerCase();
#endif
  return address;
}

String localIpAddress() {
  return WiFi.localIP().toString();
}

#if !TONGDOU_XIAOZHI_CLOUD
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
#endif

#if TONGDOU_XIAOZHI_CLOUD
void addCloudHeaders(HTTPClient& http, const String& clientId) {
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Activation-Version", "1");
  http.addHeader("Device-Id", macAddress());
  http.addHeader("Client-Id", clientId);
  http.addHeader("User-Agent", String(kBoardType) + "/" + kFirmwareVersion);
  http.addHeader("Accept-Language", "zh-CN");
}
#endif

}  // namespace

DeviceActivationService::DeviceActivationService(
    RealtimeVoiceService& realtimeVoiceService, FaceDisplay& faceDisplay,
    ServerEndpointStore& serverEndpointStore)
    : realtimeVoiceService_(realtimeVoiceService),
      faceDisplay_(faceDisplay),
      serverEndpointStore_(serverEndpointStore) {}

void DeviceActivationService::begin() {
  state_ = State::WaitingForWifi;
  consecutiveFailures_ = 0;
  scheduleRetry(kInitialDelayMs);
  Serial.print("device activation ota=");
  const String configuredUrl = serverEndpointStore_.otaUrl();
  Serial.println(configuredUrl.length() > 0 ? configuredUrl : String(kOtaUrl));
}

void DeviceActivationService::update() {
  if (!WiFi.isConnected()) {
    return;
  }

  const unsigned long now = millis();
  if (static_cast<long>(now - nextAttemptMs_) < 0) {
    return;
  }

#if TONGDOU_XIAOZHI_CLOUD
  if (state_ == State::WaitingForBinding && activationChallenge_.length() > 0) {
    confirmCloudActivation();
    return;
  }
#endif
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
#if TONGDOU_XIAOZHI_CLOUD
  if (cloudIdentityStore_.clientId().length() == 0 &&
      !cloudIdentityStore_.begin()) {
    lastError_ = "cloud_client_id_unavailable";
    Serial.println("device activation failed: cloud_client_id_unavailable");
    scheduleFailureRetry();
    return;
  }
  if (time(nullptr) < 1700000000) {
    lastError_ = "cloud_time_not_ready";
    scheduleFailureRetry();
    return;
  }
#endif
  String otaUrl = serverEndpointStore_.otaUrl();
  if (otaUrl.length() == 0) {
    otaUrl = kOtaUrl;
  }
  if (otaUrl.length() == 0) {
    lastError_ = "ota_url_not_configured";
    Serial.println("device activation waiting: ota_url_not_configured");
    scheduleFailureRetry();
    return;
  }

  HTTPClient http;
#if TONGDOU_XIAOZHI_CLOUD
  WiFiClientSecure secureClient;
  secureClient.setCACert(kXiaozhiCloudRootCa);
  const bool started = otaUrl.startsWith("https://") &&
                       http.begin(secureClient, otaUrl);
#else
  const bool started = http.begin(otaUrl);
#endif
  if (!started) {
    lastError_ = "ota_begin_failed";
    Serial.println("device activation failed: ota_begin_failed");
    scheduleFailureRetry();
    return;
  }

#if TONGDOU_XIAOZHI_CLOUD
  addCloudHeaders(http, cloudIdentityStore_.clientId());
#else
  const String deviceId = macAddress();
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Device-Id", deviceId);
  http.addHeader("Client-Id", deviceId);
#endif

  const String body = buildReportJson();
  const int status = http.POST(body);
  if (status <= 0) {
    lastError_ = "ota_post_failed";
    Serial.print("device activation failed: ota_post_failed status=");
    Serial.print(status);
    Serial.print(" error=");
    Serial.println(HTTPClient::errorToString(status));
    http.end();
    scheduleFailureRetry();
    return;
  }

  const String response = http.getString();
  http.end();

  if (status < 200 || status >= 300) {
    lastError_ = "ota_http_error";
    Serial.print("device activation failed: http status=");
    Serial.println(status);
    scheduleFailureRetry();
    return;
  }

  if (!applyResponse(response)) {
    scheduleFailureRetry();
    return;
  }

  consecutiveFailures_ = 0;
#if TONGDOU_XIAOZHI_CLOUD
  if (state_ == State::WaitingForBinding && activationChallenge_.length() > 0) {
    scheduleRetry(kBindingPollIntervalMs);
    return;
  }
#endif
  scheduleRetry(state_ == State::Activated ? kActivatedCheckIntervalMs
                                           : kSteadyRetryIntervalMs);
}

#if TONGDOU_XIAOZHI_CLOUD
void DeviceActivationService::confirmCloudActivation() {
  if (time(nullptr) < 1700000000) {
    lastError_ = "cloud_time_not_ready";
    scheduleFailureRetry();
    return;
  }

  String url = serverEndpointStore_.otaUrl();
  if (!url.startsWith("https://")) {
    lastError_ = "cloud_ota_url_invalid";
    scheduleFailureRetry();
    return;
  }
  if (!url.endsWith("/")) {
    url += "/";
  }
  url += "activate";

  WiFiClientSecure secureClient;
  secureClient.setCACert(kXiaozhiCloudRootCa);
  HTTPClient http;
  if (!http.begin(secureClient, url)) {
    lastError_ = "cloud_activation_begin_failed";
    scheduleFailureRetry();
    return;
  }
  addCloudHeaders(http, cloudIdentityStore_.clientId());
  const int status = http.POST("{}");
  http.end();

  if (status == 202) {
    lastError_ = "";
    scheduleRetry(kBindingPollIntervalMs);
    return;
  }
  if (status == 200) {
    lastError_ = "";
    activationChallenge_ = "";
    consecutiveFailures_ = 0;
    scheduleRetry(0);
    return;
  }
  lastError_ = "cloud_activation_http_error";
  Serial.print("device activation failed: cloud activation status=");
  Serial.println(status);
  scheduleFailureRetry();
}
#endif

String DeviceActivationService::buildReportJson() const {
  JsonDocument doc;
  const String deviceId = macAddress();

  doc["version"] = TONGDOU_XIAOZHI_CLOUD ? 2 : 1;
  doc["flash_size"] = ESP.getFlashChipSize();
  doc["minimum_free_heap_size"] = ESP.getMinFreeHeap();
  doc["mac_address"] = deviceId;
#if TONGDOU_XIAOZHI_CLOUD
  doc["uuid"] = cloudIdentityStore_.clientId();
#else
  doc["uuid"] = deviceId;
#endif
  doc["chip_model_name"] = ESP.getChipModel();
  doc["chip_info"]["model"] = 0;
  doc["chip_info"]["cores"] = ESP.getChipCores();
  doc["chip_info"]["revision"] = ESP.getChipRevision();
  doc["chip_info"]["features"] = 0;
  doc["application"]["name"] = kProjectName;
  doc["application"]["version"] = kFirmwareVersion;
  doc["application"]["compile_time"] = String(__DATE__) + " " + String(__TIME__);
  doc["application"]["idf_version"] = ESP.getSdkVersion();
  doc["application"]["elf_sha256"] = "";
  doc["ota"]["label"] = "app";
  doc["board"]["type"] = kBoardType;
  doc["board"]["revision"] = kBoardVersion;
  // 发布脚本核对这个完整标识，避免错主板或旧构建被当作新版本发布。
#if TONGDOU_XIAOZHI_CLOUD
  doc["board"]["firmware_identity"] =
      "tongdou-cloud-image:" TONGDOU_BOARD_TYPE ":" TONGDOU_FIRMWARE_VERSION ";";
#else
  doc["board"]["firmware_identity"] =
      "tongdou-image:" TONGDOU_BOARD_TYPE ":" TONGDOU_FIRMWARE_VERSION ";";
#endif
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

#if !TONGDOU_XIAOZHI_CLOUD
  const char* firmwareVersion = doc["firmware"]["version"] | "";
  const char* firmwareUrl = doc["firmware"]["url"] | "";
  const char* firmwareSha256 = doc["firmware"]["sha256"] | "";
  const uint32_t firmwareSize = doc["firmware"]["size"] | 0;
  if (strlen(firmwareUrl) > 0 && strlen(firmwareVersion) > 0 &&
      String(firmwareVersion) != kFirmwareVersion) {
    FirmwareUpdateOffer offer;
    offer.version = firmwareVersion;
    offer.url = firmwareUrl;
    offer.sha256 = firmwareSha256;
    offer.size = firmwareSize;
    if (!firmwareUpdateOfferPending_ ||
        firmwareUpdateOffer_.version != offer.version ||
        firmwareUpdateOffer_.url != offer.url ||
        firmwareUpdateOffer_.sha256 != offer.sha256 ||
        firmwareUpdateOffer_.size != offer.size) {
      firmwareUpdateOffer_ = offer;
      firmwareUpdateOfferPending_ = true;
    }
  }
#endif

  const char* code = doc["activation"]["code"] | "";
  if (strlen(code) > 0) {
#if TONGDOU_XIAOZHI_CLOUD
    activationChallenge_ = doc["activation"]["challenge"] | "";
#endif
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

#if TONGDOU_XIAOZHI_CLOUD
  const bool needsVoiceConfiguration =
      !voiceConfigured_ || configuredWebsocketUrl_ != websocketUrl ||
      configuredToken_ != token;
#else
  const bool needsVoiceConfiguration = !voiceConfigured_;
#endif
  if (needsVoiceConfiguration &&
      !configureVoiceBackend(String(websocketUrl), String(token))) {
    return false;
  }

  state_ = State::Activated;
#if TONGDOU_XIAOZHI_CLOUD
  activationChallenge_ = "";
  configuredWebsocketUrl_ = websocketUrl;
  configuredToken_ = token;
#endif
  activationCode_ = "";
  activationCodeEvent_ = "";
  activationCodeEventPending_ = false;
#if TONGDOU_XIAOZHI_CLOUD
  codeDisplayed_ = false;
#endif
  lastError_ = "";
  Serial.println("device activation active");
  return true;
}

bool DeviceActivationService::consumeFirmwareUpdateOffer(
    FirmwareUpdateOffer& offer) {
  if (!firmwareUpdateOfferPending_) {
    return false;
  }
  offer = firmwareUpdateOffer_;
  firmwareUpdateOfferPending_ = false;
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
#if TONGDOU_XIAOZHI_CLOUD
  if (!config.useTls) {
    lastError_ = "cloud_websocket_tls_required";
    return false;
  }
  config.rootCa = kXiaozhiCloudRootCa;
  config.clientId = cloudIdentityStore_.clientId();
#else
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
#endif

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

void DeviceActivationService::scheduleFailureRetry() {
  if (consecutiveFailures_ < UINT8_MAX) {
    ++consecutiveFailures_;
  }

  unsigned long delayMs = kSteadyRetryIntervalMs;
  switch (consecutiveFailures_) {
    case 1:
      delayMs = kFirstFailureRetryMs;
      break;
    case 2:
      delayMs = kSecondFailureRetryMs;
      break;
    case 3:
      delayMs = kThirdFailureRetryMs;
      break;
    default:
      break;
  }

  Serial.print("device activation retry failures=");
  Serial.print(consecutiveFailures_);
  Serial.print(" delay_ms=");
  Serial.println(delayMs);
  scheduleRetry(delayMs);
}

void DeviceActivationService::scheduleRetry(unsigned long delayMs) {
  nextAttemptMs_ = millis() + delayMs;
}

}  // namespace tongdou
