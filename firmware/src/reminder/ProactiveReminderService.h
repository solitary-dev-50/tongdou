#pragma once

#include <stdint.h>

namespace tongdou {

enum class ProactiveReminderType : uint8_t {
  None,
  ActivityBreak,
  Hydration,
};

struct ProactiveReminderSettings {
  bool activityEnabled = true;
  uint16_t activityIntervalMinutes = 60;
  bool hydrationEnabled = true;
  uint16_t hydrationIntervalMinutes = 45;
};

// 只根据稳定的桌前 presence 维护主动提醒的到期状态。
class ProactiveReminderService {
 public:
  static constexpr uint16_t kDefaultActivityIntervalMinutes = 60;
  static constexpr uint16_t kDefaultHydrationIntervalMinutes = 45;
  static constexpr uint16_t kAbsenceResetThresholdMinutes = 5;
  static constexpr uint16_t kCrossTypeCooldownMinutes = 5;

  explicit ProactiveReminderService(const ProactiveReminderSettings& settings);

  void begin(uint32_t nowMs);
  void update(uint32_t nowMs, bool presence);
  void setSettings(const ProactiveReminderSettings& settings);
  ProactiveReminderSettings settings() const;

  // 返回一个待送达事件，并锁住它，直到调用 delivered 或 release。
  ProactiveReminderType takeDue(uint32_t nowMs, bool deliveryAvailable);
  void delivered(ProactiveReminderType type, uint32_t nowMs);
  void release(ProactiveReminderType type);

 private:
  static constexpr uint32_t kUpdateIntervalMs = 1000;

  static uint32_t minutesToMs(uint16_t minutes);
  void resetDeskSession();
  ProactiveReminderSettings settings_;
  uint32_t lastUpdateMs_ = 0;
  uint32_t activityCycleStartedMs_ = 0;
  uint32_t hydrationCycleStartedMs_ = 0;
  uint32_t absenceStartedMs_ = 0;
  uint32_t lastDeliveryMs_ = 0;
  ProactiveReminderType lastDeliveredType_ = ProactiveReminderType::None;
  bool initialized_ = false;
  bool deskSessionActive_ = false;
  bool activityPending_ = false;
  bool hydrationPending_ = false;
  ProactiveReminderType dispatching_ = ProactiveReminderType::None;
};

}  // namespace tongdou
