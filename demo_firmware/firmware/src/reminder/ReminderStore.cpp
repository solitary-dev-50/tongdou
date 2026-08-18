#include "reminder/ReminderStore.h"

namespace tongdou {
namespace {

constexpr const char* kNamespace = "td_reminder";
constexpr const char* kRecordsKey = "records";
constexpr const char* kNextIdKey = "next_id";

}  // namespace

void ReminderStore::begin() {
  preferences_.begin(kNamespace, false);
  load();
}

const ReminderRecord* ReminderStore::records() const {
  return records_;
}

uint8_t ReminderStore::capacity() const {
  return kMaxReminders;
}

bool ReminderStore::add(uint32_t dueAt, const String& text, ReminderRecord* created) {
  if (dueAt == 0 || text.length() == 0) {
    return false;
  }

  const int slot = findReusableSlot();
  if (slot < 0) {
    return false;
  }

  ReminderRecord& record = records_[slot];
  record = {};
  record.id = nextId_++;
  if (nextId_ == 0) {
    nextId_ = 1;
  }
  record.dueAt = dueAt;
  record.active = true;
  record.completed = false;
  copyText(record, text);
  save();

  if (created != nullptr) {
    *created = record;
  }
  return true;
}

bool ReminderStore::markCompleted(uint32_t id) {
  ReminderRecord* record = findById(id);
  if (record == nullptr || !record->active || record->completed) {
    return false;
  }

  record->completed = true;
  save();
  return true;
}

bool ReminderStore::remove(uint32_t id) {
  ReminderRecord* record = findById(id);
  if (record == nullptr || !record->active) {
    return false;
  }

  *record = {};
  save();
  return true;
}

void ReminderStore::load() {
  const size_t expectedSize = sizeof(records_);
  if (preferences_.getBytesLength(kRecordsKey) == expectedSize) {
    preferences_.getBytes(kRecordsKey, records_, expectedSize);
  }

  nextId_ = preferences_.getUInt(kNextIdKey, 1);
  if (nextId_ == 0) {
    nextId_ = 1;
  }
}

void ReminderStore::save() {
  preferences_.putBytes(kRecordsKey, records_, sizeof(records_));
  preferences_.putUInt(kNextIdKey, nextId_);
}

int ReminderStore::findReusableSlot() const {
  for (uint8_t i = 0; i < kMaxReminders; ++i) {
    if (!records_[i].active) {
      return i;
    }
  }

  for (uint8_t i = 0; i < kMaxReminders; ++i) {
    if (records_[i].completed) {
      return i;
    }
  }

  return -1;
}

ReminderRecord* ReminderStore::findById(uint32_t id) {
  for (uint8_t i = 0; i < kMaxReminders; ++i) {
    if (records_[i].active && records_[i].id == id) {
      return &records_[i];
    }
  }
  return nullptr;
}

void ReminderStore::copyText(ReminderRecord& record, const String& text) {
  String clean = text;
  clean.trim();
  clean.toCharArray(record.text, sizeof(record.text));
}

}  // namespace tongdou
