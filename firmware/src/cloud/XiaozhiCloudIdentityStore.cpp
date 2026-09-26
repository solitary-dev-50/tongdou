#include "cloud/XiaozhiCloudIdentityStore.h"

#include <Preferences.h>
#include <esp_system.h>

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_xz";
constexpr const char* kClientIdKey = "client_id";

String createClientId() {
  uint8_t bytes[16];
  esp_fill_random(bytes, sizeof(bytes));
  bytes[6] = (bytes[6] & 0x0F) | 0x40;
  bytes[8] = (bytes[8] & 0x3F) | 0x80;

  char text[37];
  snprintf(text, sizeof(text),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5],
           bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11],
           bytes[12], bytes[13], bytes[14], bytes[15]);
  return String(text);
}

}  // namespace

bool XiaozhiCloudIdentityStore::begin() {
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return false;
  }

  clientId_ = preferences.getString(kClientIdKey, "");
  if (clientId_.length() == 0) {
    clientId_ = createClientId();
    if (preferences.putString(kClientIdKey, clientId_) != clientId_.length()) {
      clientId_ = "";
    }
  }
  preferences.end();
  return clientId_.length() == 36;
}

const String& XiaozhiCloudIdentityStore::clientId() const {
  return clientId_;
}

}  // namespace tongdou
