#include "system/ServerEndpointStore.h"

#include <Preferences.h>

#include "tongdou/ServerConfig.h"

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_srv";
#if TONGDOU_XIAOZHI_CLOUD
constexpr const char* kOtaUrlKey = "ota_xz";
#else
constexpr const char* kOtaUrlKey = "ota_url";
#endif

Preferences preferences;

}  // namespace

void ServerEndpointStore::begin() {
  ready_ = preferences.begin(kNamespace, false);
}

String ServerEndpointStore::otaUrl() const {
  if (!ready_) {
    return "";
  }
  return preferences.getString(kOtaUrlKey, TONGDOU_OTA_URL);
}

bool ServerEndpointStore::saveOtaUrl(const String& otaUrl) {
  if (!ready_ || otaUrl.length() == 0) {
    return false;
  }
  return preferences.putString(kOtaUrlKey, otaUrl) == otaUrl.length();
}

}  // namespace tongdou
