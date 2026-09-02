#include "system/ProactiveReminderConfigStore.h"

#include <Preferences.h>

namespace tongdou {
namespace {

constexpr const char* kNamespace = "tongdou_health";
constexpr const char* kActivityEnabledKey = "act_enabled";
constexpr const char* kActivityMinutesKey = "act_minutes";
constexpr const char* kHydrationEnabledKey = "water_enabled";
constexpr const char* kHydrationMinutesKey = "water_minutes";
constexpr uint16_t kMinimumIntervalMinutes = 1;
constexpr uint16_t kMaximumIntervalMinutes = 24 * 60;

Preferences preferences;

}  // namespace

void ProactiveReminderConfigStore::begin() {
  settings_ = {};
  ready_ = preferences.begin(kNamespace, false);
  if (!ready_) {
    return;
  }

  settings_.activityEnabled =
      preferences.getBool(kActivityEnabledKey, settings_.activityEnabled);
  settings_.activityIntervalMinutes = preferences.getUShort(
      kActivityMinutesKey, settings_.activityIntervalMinutes);
  settings_.hydrationEnabled =
      preferences.getBool(kHydrationEnabledKey, settings_.hydrationEnabled);
  settings_.hydrationIntervalMinutes = preferences.getUShort(
      kHydrationMinutesKey, settings_.hydrationIntervalMinutes);
  if (!valid(settings_)) {
    settings_ = {};
    save(settings_);
  }
}

bool ProactiveReminderConfigStore::save(
    const ProactiveReminderSettings& settings) {
  if (!ready_ || !valid(settings)) {
    return false;
  }
  const bool saved =
      preferences.putBool(kActivityEnabledKey, settings.activityEnabled) == 1 &&
      preferences.putUShort(kActivityMinutesKey, settings.activityIntervalMinutes) ==
          sizeof(settings.activityIntervalMinutes) &&
      preferences.putBool(kHydrationEnabledKey, settings.hydrationEnabled) == 1 &&
      preferences.putUShort(kHydrationMinutesKey, settings.hydrationIntervalMinutes) ==
          sizeof(settings.hydrationIntervalMinutes);
  if (saved) {
    settings_ = settings;
  }
  return saved;
}

ProactiveReminderSettings ProactiveReminderConfigStore::settings() const {
  return settings_;
}

bool ProactiveReminderConfigStore::ready() const {
  return ready_;
}

bool ProactiveReminderConfigStore::valid(
    const ProactiveReminderSettings& settings) {
  return settings.activityIntervalMinutes >= kMinimumIntervalMinutes &&
         settings.activityIntervalMinutes <= kMaximumIntervalMinutes &&
         settings.hydrationIntervalMinutes >= kMinimumIntervalMinutes &&
         settings.hydrationIntervalMinutes <= kMaximumIntervalMinutes;
}

}  // namespace tongdou
