#include "system/WifiConfigStore.h"

#include <Preferences.h>

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_net";
constexpr const char* kSsidKey = "ssid";
constexpr const char* kPasswordKey = "password";

Preferences preferences;

}  // namespace

void WifiConfigStore::begin() {
  ready_ = preferences.begin(kNamespace, false);
}

bool WifiConfigStore::load(WifiCredentials& credentials) {
  if (!ready_) {
    return false;
  }

  credentials.ssid = preferences.getString(kSsidKey, "");
  credentials.password = preferences.getString(kPasswordKey, "");
  return credentials.ssid.length() > 0;
}

void WifiConfigStore::save(const WifiCredentials& credentials) {
  if (!ready_) {
    return;
  }

  preferences.putString(kSsidKey, credentials.ssid);
  preferences.putString(kPasswordKey, credentials.password);
}

void WifiConfigStore::clear() {
  if (!ready_) {
    return;
  }

  preferences.clear();
}

}  // namespace tongdou
