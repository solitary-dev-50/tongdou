#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>

namespace {

constexpr const char* kRecoveryVersion = "1.0.0";
constexpr const char* kMainPartitionLabel = "app0";
constexpr const char* kUpdateNamespace = "tongdou_ota";
constexpr const char* kUpdatePartition = "ota_req";
constexpr const char* kWifiNamespace = "tongdou_net";
constexpr const char* kServerNamespace = "tongdou_srv";
constexpr unsigned long kWifiTimeoutMs = 30000;
constexpr unsigned long kRetryDelayMs = 30000;
constexpr size_t kDownloadBufferSize = 4096;

struct PendingUpdate {
  String manifestUrl;
  String url;
  String version;
  String sha256;
  uint32_t size = 0;
  bool attempted = false;
};

struct HttpLocation {
  String host;
  String path;
  uint16_t port = 80;
};

String serverOtaUrl();

const esp_partition_t* mainPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                  ESP_PARTITION_SUBTYPE_APP_OTA_0,
                                  kMainPartitionLabel);
}

bool isLowerHexSha256(const String& value) {
  if (value.length() != 64) {
    return false;
  }
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  return true;
}

String digestToHex(const uint8_t digest[32]) {
  static const char* kHex = "0123456789abcdef";
  String output;
  output.reserve(64);
  for (size_t i = 0; i < 32; ++i) {
    output += kHex[digest[i] >> 4];
    output += kHex[digest[i] & 0x0F];
  }
  return output;
}

bool parseHttpUrl(const String& url, HttpLocation& location) {
  if (!url.startsWith("http://")) {
    return false;
  }

  const String remainder = url.substring(7);
  const int pathIndex = remainder.indexOf('/');
  String hostPort = pathIndex >= 0 ? remainder.substring(0, pathIndex) : remainder;
  location.path = pathIndex >= 0 ? remainder.substring(pathIndex) : "/";
  const int colonIndex = hostPort.lastIndexOf(':');
  if (colonIndex > 0) {
    const long parsedPort = hostPort.substring(colonIndex + 1).toInt();
    if (parsedPort <= 0 || parsedPort > 65535) {
      return false;
    }
    location.port = static_cast<uint16_t>(parsedPort);
    hostPort = hostPort.substring(0, colonIndex);
  }
  location.host = hostPort;
  return location.host.length() > 0 && location.path.length() > 0;
}

bool sameHttpOrigin(const String& left, const String& right) {
  HttpLocation leftLocation;
  HttpLocation rightLocation;
  return parseHttpUrl(left, leftLocation) && parseHttpUrl(right, rightLocation) &&
         leftLocation.host.equalsIgnoreCase(rightLocation.host) &&
         leftLocation.port == rightLocation.port;
}

String hostHeader(const HttpLocation& location) {
  return location.port == 80
             ? location.host
             : location.host + ":" + String(location.port);
}

bool readHttpHeaders(WiFiClient& client, int& status, int& contentLength) {
  client.setTimeout(15);
  String line = client.readStringUntil('\n');
  line.trim();
  const int firstSpace = line.indexOf(' ');
  if ((!line.startsWith("HTTP/1.1 ") && !line.startsWith("HTTP/1.0 ")) ||
      firstSpace < 0) {
    Serial.print(F("recovery: response status invalid line="));
    Serial.println(line);
    return false;
  }
  status = line.substring(firstSpace + 1).toInt();

  contentLength = -1;
  while (client.connected() || client.available() > 0) {
    line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      break;
    }

    String lowerLine = line;
    lowerLine.toLowerCase();
    if (lowerLine.startsWith("content-length:")) {
      contentLength = line.substring(line.indexOf(':') + 1).toInt();
    }
  }

  return true;
}

bool loadPendingUpdate(PendingUpdate& update) {
  Preferences preferences;
  if (!preferences.begin(kUpdateNamespace, true, kUpdatePartition)) {
    return false;
  }

  const bool pending = preferences.getBool("pending", false);
  update.url = preferences.getString("url", "");
  update.manifestUrl = preferences.getString("manifest", "");
  update.version = preferences.getString("version", "");
  update.sha256 = preferences.getString("sha256", "");
  update.size = preferences.getUInt("size", 0);
  update.attempted = preferences.getBool("attempted", false);
  preferences.end();

  const esp_partition_t* partition = mainPartition();
  if (update.manifestUrl.length() == 0) {
    update.manifestUrl = serverOtaUrl();
  }
  return pending && partition != nullptr &&
         sameHttpOrigin(update.manifestUrl, update.url) &&
         update.version.length() > 0 && isLowerHexSha256(update.sha256) &&
         update.size > 0 && update.size <= partition->size;
}

bool savePendingUpdate(const PendingUpdate& update) {
  Preferences preferences;
  if (!preferences.begin(kUpdateNamespace, false, kUpdatePartition)) {
    return false;
  }
  preferences.putBool("pending", false);
  const bool saved = preferences.putString("url", update.url) == update.url.length() &&
                     preferences.putString("manifest", update.manifestUrl) ==
                         update.manifestUrl.length() &&
                     preferences.putString("version", update.version) ==
                         update.version.length() &&
                     preferences.putString("sha256", update.sha256) ==
                         update.sha256.length() &&
                     preferences.putUInt("size", update.size) == sizeof(uint32_t) &&
                     preferences.putBool("attempted", update.attempted) ==
                         sizeof(uint8_t) &&
                     preferences.putBool("pending", true) == sizeof(uint8_t);
  preferences.end();
  return saved;
}

String serverOtaUrl() {
  Preferences preferences;
  if (!preferences.begin(kServerNamespace, true)) {
    return "";
  }
  const String url = preferences.getString("ota_url", "");
  preferences.end();
  return url;
}

void clearPendingUpdate() {
  Preferences preferences;
  if (!preferences.begin(kUpdateNamespace, false, kUpdatePartition)) {
    return;
  }
  preferences.clear();
  preferences.end();
}

bool bootMainFirmware() {
  const esp_partition_t* partition = mainPartition();
  if (partition == nullptr) {
    Serial.println(F("recovery: main partition missing"));
    return false;
  }

  const esp_err_t result = esp_ota_set_boot_partition(partition);
  if (result != ESP_OK) {
    Serial.printf("recovery: main image is not bootable error=%s\n",
                  esp_err_to_name(result));
    return false;
  }

  Serial.println(F("recovery: booting main firmware"));
  delay(200);
  ESP.restart();
  return true;
}

bool connectWifi() {
  Preferences preferences;
  if (!preferences.begin(kWifiNamespace, true)) {
    Serial.println(F("recovery: wifi preferences unavailable"));
    return false;
  }
  const String ssid = preferences.getString("ssid", "");
  const String password = preferences.getString("password", "");
  preferences.end();

  if (ssid.length() == 0) {
    Serial.println(F("recovery: wifi credentials missing"));
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  const unsigned long startedAt = millis();
  while (!WiFi.isConnected() && millis() - startedAt < kWifiTimeoutMs) {
    delay(250);
  }

  if (!WiFi.isConnected()) {
    Serial.println(F("recovery: wifi connection timed out"));
    WiFi.disconnect();
    return false;
  }

  Serial.print(F("recovery: wifi connected ip="));
  Serial.println(WiFi.localIP());
  return true;
}

bool requestReplacementOffer(const PendingUpdate& failed,
                             PendingUpdate& replacement) {
  const String otaUrl = failed.manifestUrl;
  HttpLocation location;
  if (!parseHttpUrl(otaUrl, location)) {
    Serial.println(F("recovery: ota endpoint unavailable"));
    return false;
  }

  const String deviceId = WiFi.macAddress();
  String body = F("{\"application\":{\"name\":\"Tong Dou recovery\",\"version\":\"");
  body += failed.version;
  body += F("\"},\"board\":{\"type\":\"tongdou-esp32s3\"}}");

  WiFiClient client;
  client.setTimeout(15000);
  if (!client.connect(location.host.c_str(), location.port, 10000)) {
    Serial.println(F("recovery: ota endpoint connection failed"));
    return false;
  }
  client.print(F("POST "));
  client.print(location.path);
  client.print(F(" HTTP/1.1\r\nHost: "));
  client.print(hostHeader(location));
  client.print(F("\r\nContent-Type: application/json\r\nDevice-Id: "));
  client.print(deviceId);
  client.print(F("\r\nClient-Id: "));
  client.print(deviceId);
  client.print(F("\r\nDevice-Recovery: 1\r\nContent-Length: "));
  client.print(body.length());
  client.print(F("\r\nConnection: close\r\n\r\n"));
  client.print(body);

  int status = 0;
  int contentLength = -1;
  if (!readHttpHeaders(client, status, contentLength) || status != 200 ||
      contentLength <= 0 || contentLength > 8192) {
    Serial.printf("recovery: ota response rejected status=%d bytes=%d\n",
                  status, contentLength);
    client.stop();
    return false;
  }

  String response;
  response.reserve(contentLength);
  const unsigned long startedAt = millis();
  while (response.length() < static_cast<size_t>(contentLength) &&
         millis() - startedAt < 15000) {
    while (client.available() > 0 &&
           response.length() < static_cast<size_t>(contentLength)) {
      response += static_cast<char>(client.read());
    }
    delay(1);
  }
  client.stop();
  if (response.length() != static_cast<size_t>(contentLength)) {
    Serial.println(F("recovery: ota response ended early"));
    return false;
  }

  JsonDocument document;
  if (deserializeJson(document, response)) {
    Serial.println(F("recovery: ota response json invalid"));
    return false;
  }
  replacement.version = String(document["firmware"]["version"] | "");
  replacement.manifestUrl = otaUrl;
  replacement.url = String(document["firmware"]["url"] | "");
  replacement.sha256 = String(document["firmware"]["sha256"] | "");
  replacement.size = document["firmware"]["size"] | 0;
  replacement.attempted = false;

  const esp_partition_t* partition = mainPartition();
  if (replacement.version.length() == 0 ||
      replacement.version == failed.version ||
      !sameHttpOrigin(otaUrl, replacement.url) ||
      !isLowerHexSha256(replacement.sha256) || partition == nullptr ||
      replacement.size == 0 || replacement.size > partition->size) {
    Serial.println(F("recovery: no different trusted firmware available"));
    return false;
  }
  return savePendingUpdate(replacement);
}

bool installUpdate(const PendingUpdate& update) {
  const esp_partition_t* partition = mainPartition();
  if (partition == nullptr) {
    Serial.println(F("recovery: main partition missing"));
    return false;
  }

  HttpLocation location;
  if (!parseHttpUrl(update.url, location)) {
    Serial.println(F("recovery: download url invalid"));
    return false;
  }

  WiFiClient client;
  client.setTimeout(15000);
  if (!client.connect(location.host.c_str(), location.port, 10000)) {
    Serial.println(F("recovery: download connection failed"));
    return false;
  }
  client.print(F("GET "));
  client.print(location.path);
  client.print(F(" HTTP/1.1\r\nHost: "));
  client.print(hostHeader(location));
  client.print(F("\r\nConnection: close\r\n\r\n"));
  int status = 0;
  int contentLength = -1;
  if (!readHttpHeaders(client, status, contentLength) || status != 200 ||
      contentLength < 0 || static_cast<uint32_t>(contentLength) != update.size) {
    Serial.printf("recovery: download rejected status=%d expected=%u actual=%d\n",
                  status, update.size, contentLength);
    client.stop();
    return false;
  }

  esp_ota_handle_t otaHandle = 0;
  esp_err_t result = esp_ota_begin(partition, update.size, &otaHandle);
  if (result != ESP_OK) {
    Serial.printf("recovery: flash begin failed error=%s\n",
                  esp_err_to_name(result));
    client.stop();
    return false;
  }

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  if (mbedtls_sha256_starts_ret(&sha, 0) != 0) {
    Serial.println(F("recovery: sha256 begin failed"));
    mbedtls_sha256_free(&sha);
    esp_ota_abort(otaHandle);
    client.stop();
    return false;
  }

  uint8_t* buffer = static_cast<uint8_t*>(malloc(kDownloadBufferSize));
  if (buffer == nullptr) {
    Serial.println(F("recovery: download buffer allocation failed"));
    mbedtls_sha256_free(&sha);
    esp_ota_abort(otaHandle);
    client.stop();
    return false;
  }

  uint32_t written = 0;
  bool ok = true;
  unsigned long lastDataAt = millis();
  while (written < update.size) {
    const size_t available = client.available();
    if (available == 0) {
      if (!client.connected() || millis() - lastDataAt >= 15000) {
        ok = false;
        Serial.println(F("recovery: download stream ended early"));
        break;
      }
      delay(10);
      continue;
    }

    const size_t remaining = update.size - written;
    const size_t requestSize = min(min(available, kDownloadBufferSize), remaining);
    const int received = client.readBytes(buffer, requestSize);
    if (received <= 0) {
      ok = false;
      Serial.println(F("recovery: download read failed"));
      break;
    }

    lastDataAt = millis();
    if (mbedtls_sha256_update_ret(&sha, buffer, received) != 0 ||
        esp_ota_write(otaHandle, buffer, received) != ESP_OK) {
      ok = false;
      Serial.println(F("recovery: flash write failed"));
      break;
    }
    written += static_cast<uint32_t>(received);
    delay(0);
  }

  uint8_t digest[32] = {};
  if (ok && mbedtls_sha256_finish_ret(&sha, digest) != 0) {
    ok = false;
    Serial.println(F("recovery: sha256 finish failed"));
  }
  mbedtls_sha256_free(&sha);
  free(buffer);
  client.stop();

  if (!ok || written != update.size || digestToHex(digest) != update.sha256) {
    if (ok) {
      Serial.println(F("recovery: sha256 mismatch"));
    }
    esp_ota_abort(otaHandle);
    return false;
  }

  result = esp_ota_end(otaHandle);
  if (result != ESP_OK) {
    Serial.printf("recovery: image validation failed error=%s\n",
                  esp_err_to_name(result));
    return false;
  }

  PendingUpdate attempted = update;
  attempted.attempted = true;
  if (!savePendingUpdate(attempted)) {
    Serial.println(F("recovery: update attempt state save failed"));
    return false;
  }
  Serial.printf("recovery: installed version=%s bytes=%u\n",
                update.version.c_str(), written);
  return bootMainFirmware();
}

void runRecovery() {
  PendingUpdate update;
  if (!loadPendingUpdate(update)) {
    clearPendingUpdate();
    bootMainFirmware();
    return;
  }

  Serial.printf("recovery: pending version=%s bytes=%u\n",
                update.version.c_str(), update.size);
  if (!connectWifi()) {
    return;
  }
  if (update.attempted) {
    PendingUpdate replacement;
    Serial.print(F("recovery: previous main boot failed version="));
    Serial.println(update.version);
    if (!requestReplacementOffer(update, replacement)) {
      return;
    }
    update = replacement;
    Serial.print(F("recovery: replacement available version="));
    Serial.println(update.version);
  }
  installUpdate(update);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("Tong Dou recovery version=%s\n", kRecoveryVersion);
  runRecovery();
}

void loop() {
  delay(kRetryDelayMs);
  runRecovery();
}
