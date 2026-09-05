#include "cloud/FirmwareUpdateService.h"

#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "tongdou/BuildInfo.h"
#include "tongdou/ServerConfig.h"

extern "C" bool verifyRollbackLater() {
  return true;
}

namespace tongdou {
namespace {

constexpr const char* kUpdateNamespace = "tongdou_ota";
constexpr const char* kUpdatePartition = "ota_req";
constexpr const char* kRecoveryPartitionLabel = "recovery";
constexpr const char* kMainPartitionLabel = "app0";

String httpOrigin(const String& input) {
  String url = input;
  url.trim();
  if (!url.startsWith("http://")) {
    return "";
  }

  const int pathIndex = url.indexOf('/', 7);
  String origin = pathIndex >= 0 ? url.substring(0, pathIndex) : url;
  origin.toLowerCase();
  return origin;
}

bool isNumericVersion(const String& value) {
  if (value.length() == 0 || value.length() > 31) {
    return false;
  }

  bool previousWasDot = true;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c >= '0' && c <= '9') {
      previousWasDot = false;
      continue;
    }
    if (c != '.' || previousWasDot) {
      return false;
    }
    previousWasDot = true;
  }
  return !previousWasDot;
}

int compareNumericVersions(const String& left, const String& right) {
  size_t leftStart = 0;
  size_t rightStart = 0;
  while (leftStart < left.length() || rightStart < right.length()) {
    const int leftDot = left.indexOf('.', leftStart);
    const int rightDot = right.indexOf('.', rightStart);
    const size_t leftEnd = leftDot >= 0 ? static_cast<size_t>(leftDot) : left.length();
    const size_t rightEnd =
        rightDot >= 0 ? static_cast<size_t>(rightDot) : right.length();
    const uint32_t leftPart =
        leftStart < left.length() ? left.substring(leftStart, leftEnd).toInt() : 0;
    const uint32_t rightPart = rightStart < right.length()
                                   ? right.substring(rightStart, rightEnd).toInt()
                                   : 0;
    if (leftPart != rightPart) {
      return leftPart > rightPart ? 1 : -1;
    }
    leftStart = leftEnd < left.length() ? leftEnd + 1 : left.length();
    rightStart = rightEnd < right.length() ? rightEnd + 1 : right.length();
  }
  return 0;
}

bool normalizeSha256(String& value) {
  value.toLowerCase();
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

const esp_partition_t* findMainPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                  ESP_PARTITION_SUBTYPE_APP_OTA_0,
                                  kMainPartitionLabel);
}

const esp_partition_t* findRecoveryPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                  ESP_PARTITION_SUBTYPE_APP_FACTORY,
                                  kRecoveryPartitionLabel);
}

}  // namespace

FirmwareUpdateService::FirmwareUpdateService(
    ServerEndpointStore& serverEndpointStore)
    : serverEndpointStore_(serverEndpointStore) {}

void FirmwareUpdateService::begin() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
  if (running != nullptr &&
      esp_ota_get_state_partition(running, &state) == ESP_OK &&
      state == ESP_OTA_IMG_PENDING_VERIFY) {
    confirmPending_ = true;
    confirmAtMs_ = millis() + kBootConfirmationDelayMs;
    Serial.println(F("firmware update: boot confirmation pending"));
    return;
  }

  if (storedOfferPending()) {
    clearStoredOffer();
  }
}

void FirmwareUpdateService::update() {
  if (!confirmPending_ ||
      static_cast<long>(millis() - confirmAtMs_) < 0) {
    return;
  }
  confirmRunningFirmware();
}

bool FirmwareUpdateService::acceptOffer(const FirmwareUpdateOffer& offer) {
  FirmwareUpdateOffer normalizedOffer;
  if (!validateOffer(offer, normalizedOffer)) {
    Serial.print(F("firmware update: offer rejected error="));
    Serial.println(lastError_);
    return false;
  }

  if (offerPending_ && pendingOffer_.version == normalizedOffer.version &&
      pendingOffer_.url == normalizedOffer.url &&
      pendingOffer_.sha256 == normalizedOffer.sha256 &&
      pendingOffer_.size == normalizedOffer.size) {
    return true;
  }

  pendingOffer_ = normalizedOffer;
  offerPending_ = true;
  lastError_ = "";
  Serial.print(F("firmware update: offer accepted version="));
  Serial.print(pendingOffer_.version);
  Serial.print(F(" bytes="));
  Serial.println(pendingOffer_.size);
  return true;
}

bool FirmwareUpdateService::offerPending() const {
  return offerPending_;
}

bool FirmwareUpdateService::installPendingAndRestart() {
  if (!offerPending_) {
    lastError_ = "update_offer_missing";
    return false;
  }

  const esp_partition_t* recovery = findRecoveryPartition();
  if (recovery == nullptr) {
    lastError_ = "recovery_partition_missing";
    return false;
  }
  if (!savePendingOffer(pendingOffer_)) {
    return false;
  }

  const esp_err_t result = esp_ota_set_boot_partition(recovery);
  if (result != ESP_OK) {
    clearStoredOffer();
    lastError_ = "recovery_boot_select_failed";
    Serial.print(F("firmware update: recovery select failed error="));
    Serial.println(esp_err_to_name(result));
    return false;
  }

  Serial.print(F("firmware update: restarting into recovery version="));
  Serial.println(pendingOffer_.version);
  delay(200);
  ESP.restart();
  return true;
}

String FirmwareUpdateService::lastError() const {
  return lastError_;
}

bool FirmwareUpdateService::validateOffer(
    const FirmwareUpdateOffer& offer, FirmwareUpdateOffer& normalizedOffer) {
  normalizedOffer = offer;
  normalizedOffer.version.trim();
  normalizedOffer.url.trim();
  normalizedOffer.sha256.trim();

  if (!isNumericVersion(normalizedOffer.version) ||
      compareNumericVersions(normalizedOffer.version, kFirmwareVersion) <= 0) {
    lastError_ = "update_version_invalid";
    return false;
  }

  String configuredUrl = serverEndpointStore_.otaUrl();
  if (configuredUrl.length() == 0) {
    configuredUrl = TONGDOU_OTA_URL;
  }
  const String configuredOrigin = httpOrigin(configuredUrl);
  const String downloadOrigin = httpOrigin(normalizedOffer.url);
  if (configuredOrigin.length() == 0 || downloadOrigin != configuredOrigin) {
    lastError_ = "update_url_not_trusted";
    return false;
  }

  if (!normalizeSha256(normalizedOffer.sha256)) {
    lastError_ = "update_sha256_invalid";
    return false;
  }

  const esp_partition_t* main = findMainPartition();
  if (main == nullptr || normalizedOffer.size == 0 ||
      normalizedOffer.size > main->size) {
    lastError_ = "update_size_invalid";
    return false;
  }
  return true;
}

bool FirmwareUpdateService::savePendingOffer(
    const FirmwareUpdateOffer& offer) {
  String manifestUrl = serverEndpointStore_.otaUrl();
  if (manifestUrl.length() == 0) {
    manifestUrl = TONGDOU_OTA_URL;
  }
  Preferences preferences;
  if (!preferences.begin(kUpdateNamespace, false, kUpdatePartition)) {
    lastError_ = "update_store_open_failed";
    return false;
  }

  preferences.putBool("pending", false);
  const bool saved = preferences.putString("url", offer.url) == offer.url.length() &&
                     preferences.putString("version", offer.version) ==
                         offer.version.length() &&
                     preferences.putString("manifest", manifestUrl) ==
                         manifestUrl.length() &&
                     preferences.putString("sha256", offer.sha256) ==
                         offer.sha256.length() &&
                     preferences.putUInt("size", offer.size) == sizeof(uint32_t) &&
                     preferences.putBool("attempted", false) == sizeof(uint8_t) &&
                     preferences.putBool("pending", true) == sizeof(uint8_t);
  preferences.end();

  if (!saved) {
    clearStoredOffer();
    lastError_ = "update_store_write_failed";
  }
  return saved;
}

bool FirmwareUpdateService::storedOfferPending() const {
  Preferences preferences;
  if (!preferences.begin(kUpdateNamespace, false, kUpdatePartition)) {
    return false;
  }
  const bool pending = preferences.getBool("pending", false);
  preferences.end();
  return pending;
}

void FirmwareUpdateService::clearStoredOffer() {
  Preferences preferences;
  if (!preferences.begin(kUpdateNamespace, false, kUpdatePartition)) {
    return;
  }
  preferences.clear();
  preferences.end();
}

void FirmwareUpdateService::confirmRunningFirmware() {
  const esp_err_t result = esp_ota_mark_app_valid_cancel_rollback();
  if (result != ESP_OK) {
    lastError_ = "update_boot_confirmation_failed";
    Serial.print(F("firmware update: boot confirmation failed error="));
    Serial.println(esp_err_to_name(result));
    confirmAtMs_ = millis() + kBootConfirmationDelayMs;
    return;
  }

  confirmPending_ = false;
  clearStoredOffer();
  Serial.print(F("firmware update: boot confirmed version="));
  Serial.println(kFirmwareVersion);
}

}  // namespace tongdou
