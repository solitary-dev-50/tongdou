#include "system/DeviceLocaleStore.h"

#include <Preferences.h>

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_device";
constexpr const char* kLocaleKey = "locale";
constexpr const char* kDefaultLocale = "en-GB";

Preferences preferences;

}  // namespace

void DeviceLocaleStore::begin() {
  locale_ = kDefaultLocale;
  ready_ = preferences.begin(kNamespace, false);
  if (!ready_) {
    return;
  }

  if (!preferences.isKey(kLocaleKey)) {
    preferences.putString(kLocaleKey, kDefaultLocale);
    return;
  }

  const String saved = preferences.getString(kLocaleKey, "");
  if (supported(saved)) {
    locale_ = saved;
    return;
  }

  preferences.putString(kLocaleKey, kDefaultLocale);
}

String DeviceLocaleStore::locale() const {
  return locale_.length() > 0 ? locale_ : String(kDefaultLocale);
}

bool DeviceLocaleStore::save(const String& locale) {
  if (!ready_ || !supported(locale)) {
    return false;
  }

  if (preferences.putString(kLocaleKey, locale) == 0) {
    return false;
  }
  locale_ = locale;
  return true;
}

bool DeviceLocaleStore::ready() const {
  return ready_;
}

const char* DeviceLocaleStore::defaultLocale() {
  return kDefaultLocale;
}

bool DeviceLocaleStore::supported(const String& locale) {
  return locale == "en-GB" || locale == "en-CA" || locale == "fr-FR" ||
         locale == "fr-CA" || locale == "nl-NL" || locale == "ru-RU" ||
         locale == "zh-CN";
}

}  // namespace tongdou
