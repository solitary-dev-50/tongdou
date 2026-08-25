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

struct ReminderRecord {
  uint32_t id = 0;
  uint32_t dueAt = 0;
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
  bool markCompleted(uint32_t id);
  bool remove(uint32_t id);

 private:
  void load();
  bool saveMetadata();
  bool loadDisplayBitmap(uint8_t slot, ReminderRecord& record) const;
  bool saveDisplayBitmap(uint8_t slot, uint32_t reminderId,
                         const uint8_t* displayBitmap);
  int findReusableSlot() const;
  ReminderRecord* findById(uint32_t id);
  void copyText(ReminderRecord& record, const String& text);

  Preferences preferences_;
  const esp_partition_t* bitmapPartition_ = nullptr;
  ReminderRecord* records_ = nullptr;
  uint32_t nextId_ = 1;
};

}  // namespace tongdou
