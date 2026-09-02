#pragma once

#include "reminder/ProactiveReminderService.h"

namespace tongdou {

// 只保存主动活动和喝水提醒的用户设置。
class ProactiveReminderConfigStore {
 public:
  void begin();
  bool save(const ProactiveReminderSettings& settings);
  ProactiveReminderSettings settings() const;
  bool ready() const;

  static bool valid(const ProactiveReminderSettings& settings);

 private:
  bool ready_ = false;
  ProactiveReminderSettings settings_;
};

}  // namespace tongdou
