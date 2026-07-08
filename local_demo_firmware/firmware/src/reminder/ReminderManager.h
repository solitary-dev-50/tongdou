#pragma once

#include "reminder/ReminderStore.h"
#include "scenario/ScenarioEvent.h"
#include "system/TimeService.h"

namespace tongdou {

class ReminderManager {
 public:
  ReminderManager(TimeService& timeService, ReminderStore& store);

  void begin();
  bool update(ScenarioEvent& event);

 private:
  TimeService& timeService_;
  ReminderStore& store_;
};

}  // namespace tongdou
