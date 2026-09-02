#include "reminder/ProactiveReminderService.h"

namespace tongdou {

ProactiveReminderService::ProactiveReminderService(
    const ProactiveReminderSettings& settings)
    : settings_(settings) {}

void ProactiveReminderService::begin(uint32_t nowMs) {
  initialized_ = true;
  lastUpdateMs_ = nowMs;
  activityCycleStartedMs_ = 0;
  hydrationCycleStartedMs_ = 0;
  absenceStartedMs_ = 0;
  lastDeliveryMs_ = 0;
  lastDeliveredType_ = ProactiveReminderType::None;
  deskSessionActive_ = false;
  activityPending_ = false;
  hydrationPending_ = false;
  dispatching_ = ProactiveReminderType::None;
}

void ProactiveReminderService::update(uint32_t nowMs, bool presence) {
  if (!initialized_) {
    begin(nowMs);
  }
  if (nowMs - lastUpdateMs_ < kUpdateIntervalMs) {
    return;
  }
  lastUpdateMs_ = nowMs;

  if (presence) {
    absenceStartedMs_ = 0;
    if (!deskSessionActive_) {
      deskSessionActive_ = true;
      activityCycleStartedMs_ = nowMs;
      hydrationCycleStartedMs_ = nowMs;
    }
  } else if (deskSessionActive_) {
    if (absenceStartedMs_ == 0) {
      absenceStartedMs_ = nowMs;
    } else if (nowMs - absenceStartedMs_ >=
               minutesToMs(kAbsenceResetThresholdMinutes)) {
      resetDeskSession();
    }
  }

  if (!deskSessionActive_) {
    return;
  }
  if (settings_.activityEnabled && nowMs - activityCycleStartedMs_ >=
                                       minutesToMs(settings_.activityIntervalMinutes)) {
    activityPending_ = true;
  }
  if (settings_.hydrationEnabled && nowMs - hydrationCycleStartedMs_ >=
                                       minutesToMs(settings_.hydrationIntervalMinutes)) {
    hydrationPending_ = true;
  }
}

void ProactiveReminderService::setSettings(
    const ProactiveReminderSettings& settings) {
  settings_ = settings;
  if (!settings_.activityEnabled) {
    activityPending_ = false;
    if (dispatching_ == ProactiveReminderType::ActivityBreak) {
      dispatching_ = ProactiveReminderType::None;
    }
  }
  if (!settings_.hydrationEnabled) {
    hydrationPending_ = false;
    if (dispatching_ == ProactiveReminderType::Hydration) {
      dispatching_ = ProactiveReminderType::None;
    }
  }
}

ProactiveReminderSettings ProactiveReminderService::settings() const {
  return settings_;
}

ProactiveReminderType ProactiveReminderService::takeDue(uint32_t nowMs,
                                                         bool deliveryAvailable) {
  if (!deliveryAvailable || dispatching_ != ProactiveReminderType::None) {
    return ProactiveReminderType::None;
  }

  ProactiveReminderType nextType = ProactiveReminderType::None;
  if (activityPending_ && hydrationPending_) {
    // 两项都在等待时，把下一个跨类型时隙留给上次没播的那一项。
    nextType = lastDeliveredType_ == ProactiveReminderType::ActivityBreak
                   ? ProactiveReminderType::Hydration
                   : ProactiveReminderType::ActivityBreak;
  } else if (activityPending_) {
    nextType = ProactiveReminderType::ActivityBreak;
  } else if (hydrationPending_) {
    nextType = ProactiveReminderType::Hydration;
  }
  if (nextType == ProactiveReminderType::None) {
    return ProactiveReminderType::None;
  }

  const bool switchesType = lastDeliveredType_ != ProactiveReminderType::None &&
                            nextType != lastDeliveredType_;
  // 同类型按自己的周期重启；只有切换类型才防止紧接着轰炸用户。
  if (switchesType && lastDeliveryMs_ != 0 &&
      nowMs - lastDeliveryMs_ < minutesToMs(kCrossTypeCooldownMinutes)) {
    return ProactiveReminderType::None;
  }

  dispatching_ = nextType;
  return dispatching_;
}

void ProactiveReminderService::delivered(ProactiveReminderType type,
                                         uint32_t nowMs) {
  if (dispatching_ != type || type == ProactiveReminderType::None) {
    return;
  }
  if (type == ProactiveReminderType::ActivityBreak) {
    activityPending_ = false;
    activityCycleStartedMs_ = nowMs;
  } else if (type == ProactiveReminderType::Hydration) {
    hydrationPending_ = false;
    hydrationCycleStartedMs_ = nowMs;
  }
  lastDeliveryMs_ = nowMs;
  lastDeliveredType_ = type;
  dispatching_ = ProactiveReminderType::None;
}

void ProactiveReminderService::release(ProactiveReminderType type) {
  if (dispatching_ == type) {
    dispatching_ = ProactiveReminderType::None;
  }
}

uint32_t ProactiveReminderService::minutesToMs(uint16_t minutes) {
  return static_cast<uint32_t>(minutes) * 60U * 1000U;
}

void ProactiveReminderService::resetDeskSession() {
  deskSessionActive_ = false;
  activityCycleStartedMs_ = 0;
  hydrationCycleStartedMs_ = 0;
  absenceStartedMs_ = 0;
  activityPending_ = false;
  hydrationPending_ = false;
  dispatching_ = ProactiveReminderType::None;
}

}  // namespace tongdou
