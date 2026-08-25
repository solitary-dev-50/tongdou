#include "reminder/ReminderManager.h"

namespace tongdou {

ReminderManager::ReminderManager(TimeService& timeService, ReminderStore& store)
    : timeService_(timeService), store_(store) {}

void ReminderManager::begin() {}

bool ReminderManager::update(ReminderRecord& dueReminder) {
  if (!timeService_.ready()) {
    return false;
  }

  const uint32_t now = static_cast<uint32_t>(timeService_.now());
  const ReminderRecord* records = store_.records();
  for (uint8_t i = 0; i < store_.capacity(); ++i) {
    const ReminderRecord& record = records[i];
    if (!record.active || record.completed || record.dueAt > now) {
      continue;
    }

    dueReminder = record;
    if (!store_.markCompleted(record.id)) {
      return false;
    }
    Serial.print("reminder due id=");
    Serial.print(record.id);
    Serial.print(" text=");
    Serial.println(record.text);
    return true;
  }

  return false;
}

}  // namespace tongdou
