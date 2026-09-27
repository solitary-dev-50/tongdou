#include "system/WifiConfigStore.h"

#include <Preferences.h>

#include "tongdou/ServerConfig.h"

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_net";
constexpr const char* kSsidKey = "ssid";
constexpr const char* kPasswordKey = "password";
constexpr const char* kCountKey = "count";
constexpr const char* kPreferredKey = "preferred";
#if TONGDOU_XIAOZHI_CLOUD
constexpr const char* kServerUrlPrefix = "xz";
#else
constexpr const char* kServerUrlPrefix = "url";
#endif

Preferences preferences;

String slotKey(const char* prefix, uint8_t index) {
  return String(prefix) + String(index);
}

bool writeSlot(uint8_t index, const WifiCredentials& credentials) {
  const String ssidKey = slotKey("ssid", index);
  const String passwordKey = slotKey("pass", index);
  const String serverUrlKey = slotKey(kServerUrlPrefix, index);
  preferences.putString(ssidKey.c_str(), credentials.ssid);
  preferences.putString(passwordKey.c_str(), credentials.password);
  preferences.putString(serverUrlKey.c_str(), credentials.serverUrl);
  return preferences.getString(ssidKey.c_str(), "") == credentials.ssid &&
         preferences.getString(passwordKey.c_str(), "") == credentials.password &&
         preferences.getString(serverUrlKey.c_str(), "") == credentials.serverUrl;
}

void copySlot(uint8_t from, uint8_t to) {
  for (const char* prefix : {"ssid", "pass", "url", "xz"}) {
    const String value = preferences.getString(slotKey(prefix, from).c_str(), "");
    preferences.putString(slotKey(prefix, to).c_str(), value);
  }
}

void eraseSlot(uint8_t index) {
  for (const char* prefix : {"ssid", "pass", "url", "xz"}) {
    preferences.remove(slotKey(prefix, index).c_str());
  }
}

}  // namespace

void WifiConfigStore::begin(const String& currentServerUrl) {
  ready_ = preferences.begin(kNamespace, false);
  if (!ready_ || preferences.getUChar(kCountKey, 0) != 0) {
    return;
  }

  // 旧固件只保存一组；第一次升级时把它搬进第 0 个位置。
  WifiCredentials old;
  old.ssid = preferences.getString(kSsidKey, "");
  if (old.ssid.length() == 0) {
    return;
  }
  old.password = preferences.getString(kPasswordKey, "");
  old.serverUrl = currentServerUrl;
  if (!writeSlot(0, old)) {
    eraseSlot(0);
    return;
  }
  preferences.putUChar(kCountKey, 1);
  preferences.putUChar(kPreferredKey, 0);
}

uint8_t WifiConfigStore::count() const {
  return ready_ ? min<uint8_t>(preferences.getUChar(kCountKey, 0), kMaxNetworks) : 0;
}

uint8_t WifiConfigStore::preferredIndex() const {
  const uint8_t savedCount = count();
  if (savedCount == 0) {
    return 0;
  }
  const uint8_t preferred = preferences.getUChar(kPreferredKey, 0);
  return preferred < savedCount ? preferred : 0;
}

bool WifiConfigStore::loadAt(uint8_t index, WifiCredentials& credentials) const {
  if (index >= count()) {
    return false;
  }
  credentials.ssid = preferences.getString(slotKey("ssid", index).c_str(), "");
  credentials.password = preferences.getString(slotKey("pass", index).c_str(), "");
  credentials.serverUrl =
      preferences.getString(slotKey(kServerUrlPrefix, index).c_str(), "");
  return credentials.ssid.length() > 0;
}

bool WifiConfigStore::save(const WifiCredentials& credentials, uint8_t& index) {
  if (!ready_ || credentials.ssid.length() == 0) {
    return false;
  }
  const uint8_t savedCount = count();
  WifiCredentials existing;
  for (uint8_t i = 0; i < savedCount; ++i) {
    if (loadAt(i, existing) && existing.ssid == credentials.ssid) {
      index = i;
      return writeSlot(index, credentials);
    }
  }
  if (savedCount >= kMaxNetworks) {
    return false;
  }
  index = savedCount;
  if (!writeSlot(index, credentials)) {
    eraseSlot(index);
    return false;
  }
  preferences.putUChar(kCountKey, savedCount + 1);
  return true;
}

bool WifiConfigStore::remove(const String& ssid) {
  const uint8_t savedCount = count();
  const uint8_t oldPreferred = preferredIndex();
  WifiCredentials credentials;
  uint8_t removed = savedCount;
  for (uint8_t i = 0; i < savedCount; ++i) {
    if (loadAt(i, credentials) && credentials.ssid == ssid) {
      removed = i;
      break;
    }
  }
  if (removed == savedCount) {
    return false;
  }
  for (uint8_t i = removed; i + 1 < savedCount; ++i) {
    copySlot(i + 1, i);
  }
  eraseSlot(savedCount - 1);
  preferences.putUChar(kCountKey, savedCount - 1);
  if (savedCount == 1) {
    preferences.remove(kSsidKey);
    preferences.remove(kPasswordKey);
    preferences.putUChar(kPreferredKey, 0);
  } else {
    const uint8_t nextPreferred = oldPreferred > removed ? oldPreferred - 1 :
                                  oldPreferred == removed ? 0 : oldPreferred;
    markConnected(nextPreferred);
  }
  return true;
}

void WifiConfigStore::markConnected(uint8_t index) {
  WifiCredentials credentials;
  if (!loadAt(index, credentials)) {
    return;
  }
  preferences.putUChar(kPreferredKey, index);
  // 救援程序仍读取旧键，始终让它拿到最近连通的网络。
  preferences.putString(kSsidKey, credentials.ssid);
  preferences.putString(kPasswordKey, credentials.password);
}

void WifiConfigStore::clear() {
  if (ready_) {
    preferences.clear();
  }
}

}  // namespace tongdou
