#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>

namespace tongdou {

constexpr uint8_t kMaxReminders = 12;
constexpr size_t kReminderTextLength = 96;

struct ReminderRecord {
  uint32_t id = 0;
  uint32_t dueAt = 0;
  bool active = false;
  bool completed = false;
  char text[kReminderTextLength] = {};
};

class ReminderStore {
 public:
  void begin();

  const ReminderRecord* records() const;
  uint8_t capacity() const;

  bool add(uint32_t dueAt, const String& text, ReminderRecord* created = nullptr);
  bool markCompleted(uint32_t id);
  bool remove(uint32_t id);

 private:
  void load();
  void save();
  int findReusableSlot() const;
  ReminderRecord* findById(uint32_t id);
  void copyText(ReminderRecord& record, const String& text);

  Preferences preferences_;
  ReminderRecord records_[kMaxReminders];
  uint32_t nextId_ = 1;
};

}  // namespace tongdou
