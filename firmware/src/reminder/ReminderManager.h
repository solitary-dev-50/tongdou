#pragma once

#include "reminder/ReminderStore.h"
#include "system/TimeService.h"

namespace tongdou {

class ReminderManager {
 public:
  ReminderManager(TimeService& timeService, ReminderStore& store);

  void begin();
  bool update(ReminderRecord& dueReminder);
  bool markDelivered(uint32_t reminderId);

 private:
  TimeService& timeService_;
  ReminderStore& store_;
};

}  // namespace tongdou
