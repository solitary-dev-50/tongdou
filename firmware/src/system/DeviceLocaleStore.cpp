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
  ready_ = preferences.begin(kNamespace, false);
}

String DeviceLocaleStore::locale() const {
  if (!ready_) {
    return kDefaultLocale;
  }

  const String saved = preferences.getString(kLocaleKey, kDefaultLocale);
  return supported(saved) ? saved : kDefaultLocale;
}

bool DeviceLocaleStore::save(const String& locale) {
  if (!ready_ || !supported(locale)) {
    return false;
  }

  return preferences.putString(kLocaleKey, locale) > 0;
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
