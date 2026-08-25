#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <esp_partition.h>
#include <stdint.h>

namespace tongdou {

constexpr uint8_t kMaxReminders = 12;
constexpr size_t kReminderTextLength = 96;
constexpr uint16_t kReminderBitmapWidth = 128;
constexpr uint16_t kReminderBitmapHeight = 48;
constexpr size_t kReminderBitmapSize =
    (kReminderBitmapWidth * kReminderBitmapHeight) / 8;

enum class ReminderOccurrenceKind : uint8_t {
  OneShot = 0,
  Advance = 1,
  Snooze = 2,
  Punctual = 3,
};

const char* reminderOccurrenceKindName(ReminderOccurrenceKind kind);

struct ReminderRecord {
  uint32_t id = 0;
  uint32_t eventId = 0;
  uint32_t dueAt = 0;
  ReminderOccurrenceKind kind = ReminderOccurrenceKind::OneShot;
  bool active = false;
  bool completed = false;
  char text[kReminderTextLength] = {};
  uint16_t displayBitmapLength = 0;
  uint8_t displayBitmap[kReminderBitmapSize] = {};

  bool hasDisplayBitmap() const {
    return displayBitmapLength == kReminderBitmapSize;
  }
};

class ReminderStore {
 public:
  void begin();

  const ReminderRecord* records() const;
  uint8_t capacity() const;

  bool add(uint32_t dueAt, const String& text, ReminderRecord* created = nullptr);
  bool add(uint32_t dueAt, const String& text, const uint8_t* displayBitmap,
           size_t displayBitmapLength, ReminderRecord* created = nullptr);
  bool addEvent(uint32_t eventAt, const String& punctualText,
                const uint8_t* punctualDisplayBitmap,
                size_t punctualDisplayBitmapLength, uint32_t advanceAt,
                const String& advanceText,
                const uint8_t* advanceDisplayBitmap,
                size_t advanceDisplayBitmapLength,
                ReminderRecord* punctualCreated = nullptr,
                ReminderRecord* advanceCreated = nullptr);
  bool snooze(uint32_t sourceReminderId, uint32_t dueAt,
               ReminderRecord* created = nullptr);
  bool markCompleted(uint32_t id);
  bool remove(uint32_t id);
  const ReminderRecord* find(uint32_t id) const;

 private:
  void load();
  bool saveMetadata();
  bool loadDisplayBitmap(uint8_t slot, ReminderRecord& record) const;
  bool saveDisplayBitmap(uint8_t slot, uint32_t reminderId,
                         const uint8_t* displayBitmap);
  int findReusableSlot() const;
  bool findReusableSlots(uint8_t* slots, uint8_t count) const;
  uint32_t takeNextId();
  void initializeRecord(ReminderRecord& record, uint32_t id,
                        uint32_t eventId, uint32_t dueAt,
                        ReminderOccurrenceKind kind, const String& text);
  ReminderRecord* findById(uint32_t id);
  void copyText(ReminderRecord& record, const String& text);

  Preferences preferences_;
  const esp_partition_t* bitmapPartition_ = nullptr;
  ReminderRecord* records_ = nullptr;
  uint32_t nextId_ = 1;
};

}  // namespace tongdou
