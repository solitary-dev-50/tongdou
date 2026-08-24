#include "resources/LocaleResourceUpdateService.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

namespace tongdou {
namespace {

constexpr uint16_t kSupportedFormatVersion = 1;
constexpr unsigned long kDownloadStallTimeoutMs = 10000;

}  // namespace

LocaleResourceUpdateService::LocaleResourceUpdateService(
    LocaleResourceStore& store)
    : store_(store) {}

void LocaleResourceUpdateService::setEndpoint(const String& baseUrl,
                                              uint16_t formatVersion) {
  String normalized = baseUrl;
  normalized.trim();
  while (normalized.endsWith("/")) {
    normalized.remove(normalized.length() - 1);
  }
  if (baseUrl_ == normalized && formatVersion_ == formatVersion) {
    return;
  }

  baseUrl_ = normalized;
  formatVersion_ = formatVersion;
  Serial.print(F("locale update endpoint="));
  Serial.print(baseUrl_);
  Serial.print(F(" format="));
  Serial.println(formatVersion_);
}

LocaleResourceUpdateResult LocaleResourceUpdateService::downloadAndInstall(
    const String& locale) {
  if (baseUrl_.length() == 0) {
    return fail("locale_resource_endpoint_missing");
  }
  if (!baseUrl_.startsWith("http://")) {
    return fail("locale_resource_endpoint_unsupported");
  }
  if (formatVersion_ != kSupportedFormatVersion) {
    return fail("locale_resource_format_unsupported");
  }
  if (WiFi.status() != WL_CONNECTED) {
    return fail("locale_resource_network_unavailable");
  }
  if (store_.capacity() == 0) {
    return fail("locale_resource_partition_missing");
  }

  const String url = baseUrl_ + "/" + locale + ".bin";
  WiFiClient client;
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(kDownloadStallTimeoutMs);
  if (!http.begin(client, url)) {
    return fail("locale_resource_http_begin_failed");
  }

  Serial.print(F("locale update download start locale="));
  Serial.print(locale);
  Serial.print(F(" url="));
  Serial.println(url);

  const int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) {
    http.end();
    return fail(String("locale_resource_http_status:") + statusCode);
  }

  const int contentLength = http.getSize();
  if (contentLength <= 0 ||
      static_cast<uint32_t>(contentLength) > store_.capacity()) {
    http.end();
    return fail("locale_resource_content_length_invalid");
  }

  const size_t psramFreeBefore =
      heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  uint8_t* buffer = static_cast<uint8_t*>(heap_caps_malloc(
      static_cast<size_t>(contentLength), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (buffer == nullptr) {
    http.end();
    return fail("locale_resource_psram_allocation_failed");
  }
  Serial.print(F("locale update psram free_before="));
  Serial.print(psramFreeBefore);
  Serial.print(F(" free_after="));
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  Serial.print(F(" allocated="));
  Serial.println(contentLength);

  WiFiClient* stream = http.getStreamPtr();
  size_t downloaded = 0;
  unsigned long lastProgressMs = millis();
  while (downloaded < static_cast<size_t>(contentLength)) {
    const int available = stream->available();
    if (available > 0) {
      const size_t remaining = static_cast<size_t>(contentLength) - downloaded;
      const size_t requested =
          min<size_t>(static_cast<size_t>(available), remaining);
      const int received = stream->read(buffer + downloaded, requested);
      if (received > 0) {
        downloaded += static_cast<size_t>(received);
        lastProgressMs = millis();
        continue;
      }
    }

    if (!http.connected() ||
        millis() - lastProgressMs > kDownloadStallTimeoutMs) {
      break;
    }
    delay(1);
  }
  http.end();

  if (downloaded != static_cast<size_t>(contentLength)) {
    heap_caps_free(buffer);
    return fail("locale_resource_download_incomplete", downloaded);
  }

  Serial.print(F("locale update downloaded bytes="));
  Serial.println(downloaded);
  const bool installed = store_.installPack(buffer, downloaded, locale);
  heap_caps_free(buffer);
  if (!installed) {
    return fail(store_.lastError(), downloaded);
  }

  Serial.print(F("locale update installed locale="));
  Serial.print(store_.locale());
  Serial.print(F(" version="));
  Serial.println(store_.version());
  return {true, downloaded, ""};
}

const String& LocaleResourceUpdateService::baseUrl() const {
  return baseUrl_;
}

uint16_t LocaleResourceUpdateService::formatVersion() const {
  return formatVersion_;
}

LocaleResourceUpdateResult LocaleResourceUpdateService::fail(
    const String& error, size_t downloadedBytes) {
  Serial.print(F("locale update failed error="));
  Serial.println(error);
  return {false, downloadedBytes, error};
}

}  // namespace tongdou
