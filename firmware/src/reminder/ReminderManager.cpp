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
  const ReminderRecord* nextDue = nullptr;
  for (uint8_t i = 0; i < store_.capacity(); ++i) {
    const ReminderRecord& record = records[i];
    if (!record.active || record.completed || record.dueAt > now) {
      continue;
    }
    if (nextDue == nullptr || record.dueAt < nextDue->dueAt ||
        (record.dueAt == nextDue->dueAt && record.id < nextDue->id)) {
      nextDue = &record;
    }
  }

  if (nextDue == nullptr) {
    return false;
  }

  dueReminder = *nextDue;
  Serial.print(F("reminder due selected id="));
  Serial.print(nextDue->id);
  Serial.print(F(" event="));
  Serial.print(nextDue->eventId);
  Serial.print(F(" kind="));
  Serial.print(reminderOccurrenceKindName(nextDue->kind));
  Serial.print(F(" text="));
  Serial.println(nextDue->text);
  return true;
}

bool ReminderManager::markDelivered(uint32_t reminderId) {
  if (!store_.markCompleted(reminderId)) {
    return false;
  }
  Serial.print(F("reminder delivered id="));
  Serial.println(reminderId);
  return true;
}

}  // namespace tongdou
