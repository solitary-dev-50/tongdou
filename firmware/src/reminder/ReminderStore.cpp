#include "reminder/ReminderStore.h"

#include <cstring>
#include <new>

#include <esp_heap_caps.h>

namespace tongdou {
namespace {

constexpr const char* kNamespace = "td_reminder";
constexpr const char* kRecordsKey = "records";
constexpr const char* kNextIdKey = "next_id";
constexpr char kBitmapPartitionLabel[] = "reminder";
constexpr esp_partition_subtype_t kBitmapPartitionSubtype =
    static_cast<esp_partition_subtype_t>(0x41);
constexpr size_t kRecordsBytes = sizeof(ReminderRecord) * kMaxReminders;
constexpr size_t kFlashSectorSize = 4096;
constexpr uint8_t kBitmapSlotsPerSector = 3;
constexpr uint32_t kBitmapMagic = 0x54445242;

struct ReminderMetadata {
  uint32_t id = 0;
  uint32_t dueAt = 0;
  bool active = false;
  bool completed = false;
  char text[kReminderTextLength] = {};
};

struct ReminderBitmapEntry {
  uint32_t magic = 0;
  uint32_t reminderId = 0;
  uint32_t checksum = 0;
  uint8_t bitmap[kReminderBitmapSize] = {};
};

static_assert(sizeof(ReminderMetadata) == 108,
              "reminder metadata schema changed");
static_assert(sizeof(ReminderRecord) == 876,
              "runtime reminder schema changed unexpectedly");
static_assert(sizeof(ReminderBitmapEntry) == 780,
              "reminder bitmap entry schema changed");

uint32_t bitmapChecksum(uint32_t reminderId, const uint8_t* bitmap) {
  uint32_t hash = 2166136261U;
  const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&reminderId);
  for (size_t index = 0; index < sizeof(reminderId); ++index) {
    hash = (hash ^ idBytes[index]) * 16777619U;
  }
  for (size_t index = 0; index < kReminderBitmapSize; ++index) {
    hash = (hash ^ bitmap[index]) * 16777619U;
  }
  return hash;
}

size_t bitmapSectorOffset(uint8_t slot) {
  return static_cast<size_t>(slot / kBitmapSlotsPerSector) * kFlashSectorSize;
}

size_t bitmapEntryOffsetInSector(uint8_t slot) {
  return static_cast<size_t>(slot % kBitmapSlotsPerSector) *
         sizeof(ReminderBitmapEntry);
}

}  // namespace

void ReminderStore::begin() {
  if (records_ == nullptr) {
    records_ = static_cast<ReminderRecord*>(heap_caps_malloc(
        kRecordsBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (records_ != nullptr) {
      for (uint8_t index = 0; index < kMaxReminders; ++index) {
        new (&records_[index]) ReminderRecord();
      }
    }
  }
  if (records_ == nullptr) {
    Serial.println(F("reminder store psram allocation failed"));
    return;
  }

  preferences_.begin(kNamespace, false);
  bitmapPartition_ = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, kBitmapPartitionSubtype,
      kBitmapPartitionLabel);
  load();
  Serial.print(F("reminder store psram bytes="));
  Serial.print(kRecordsBytes);
  Serial.print(F(" bitmap_partition="));
  Serial.println(bitmapPartition_ != nullptr ? 1 : 0);
}

const ReminderRecord* ReminderStore::records() const {
  return records_;
}

uint8_t ReminderStore::capacity() const {
  return records_ == nullptr ? 0 : kMaxReminders;
}

bool ReminderStore::add(uint32_t dueAt, const String& text,
                        ReminderRecord* created) {
  return add(dueAt, text, nullptr, 0, created);
}

bool ReminderStore::add(uint32_t dueAt, const String& text,
                        const uint8_t* displayBitmap,
                        size_t displayBitmapLength,
                        ReminderRecord* created) {
  if (records_ == nullptr || dueAt == 0 || text.length() == 0) {
    return false;
  }
  if ((displayBitmap == nullptr && displayBitmapLength != 0) ||
      (displayBitmap != nullptr &&
       displayBitmapLength != kReminderBitmapSize)) {
    return false;
  }
  if (displayBitmap != nullptr && bitmapPartition_ == nullptr) {
    return false;
  }

  const int slot = findReusableSlot();
  if (slot < 0) {
    return false;
  }

  ReminderRecord& record = records_[slot];
  const ReminderRecord previous = record;
  const uint32_t previousNextId = nextId_;
  record = {};
  record.id = nextId_++;
  if (nextId_ == 0) {
    nextId_ = 1;
  }
  record.dueAt = dueAt;
  record.active = true;
  copyText(record, text);

  if (displayBitmap != nullptr) {
    if (!saveDisplayBitmap(static_cast<uint8_t>(slot), record.id,
                           displayBitmap)) {
      record = previous;
      nextId_ = previousNextId;
      return false;
    }
    memcpy(record.displayBitmap, displayBitmap, kReminderBitmapSize);
    record.displayBitmapLength = kReminderBitmapSize;
  }
  if (!saveMetadata()) {
    record = previous;
    nextId_ = previousNextId;
    return false;
  }

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
  if (saveMetadata()) {
    return true;
  }
  record->completed = false;
  return false;
}

bool ReminderStore::remove(uint32_t id) {
  ReminderRecord* record = findById(id);
  if (record == nullptr || !record->active) {
    return false;
  }

  const ReminderRecord previous = *record;
  *record = {};
  if (saveMetadata()) {
    return true;
  }
  *record = previous;
  return false;
}

void ReminderStore::load() {
  if (records_ == nullptr) {
    return;
  }
  for (uint8_t index = 0; index < kMaxReminders; ++index) {
    records_[index] = {};
  }

  nextId_ = preferences_.getUInt(kNextIdKey, 1);
  if (nextId_ == 0) {
    nextId_ = 1;
  }

  ReminderMetadata metadata[kMaxReminders] = {};
  if (preferences_.getBytesLength(kRecordsKey) != sizeof(metadata) ||
      preferences_.getBytes(kRecordsKey, metadata, sizeof(metadata)) !=
          sizeof(metadata)) {
    return;
  }

  for (uint8_t index = 0; index < kMaxReminders; ++index) {
    ReminderRecord& record = records_[index];
    record.id = metadata[index].id;
    record.dueAt = metadata[index].dueAt;
    record.active = metadata[index].active;
    record.completed = metadata[index].completed;
    memcpy(record.text, metadata[index].text, sizeof(record.text));
    if (record.active && loadDisplayBitmap(index, record)) {
      Serial.print(F("reminder bitmap loaded id="));
      Serial.println(record.id);
    }
  }
}

bool ReminderStore::saveMetadata() {
  if (records_ == nullptr) {
    return false;
  }

  ReminderMetadata metadata[kMaxReminders] = {};
  for (uint8_t index = 0; index < kMaxReminders; ++index) {
    metadata[index].id = records_[index].id;
    metadata[index].dueAt = records_[index].dueAt;
    metadata[index].active = records_[index].active;
    metadata[index].completed = records_[index].completed;
    memcpy(metadata[index].text, records_[index].text,
           sizeof(metadata[index].text));
  }

  const size_t recordsWritten =
      preferences_.putBytes(kRecordsKey, metadata, sizeof(metadata));
  const size_t idWritten = preferences_.putUInt(kNextIdKey, nextId_);
  return recordsWritten == sizeof(metadata) && idWritten == sizeof(nextId_);
}

bool ReminderStore::loadDisplayBitmap(uint8_t slot,
                                      ReminderRecord& record) const {
  if (bitmapPartition_ == nullptr || slot >= kMaxReminders) {
    return false;
  }

  ReminderBitmapEntry entry;
  const size_t offset = bitmapSectorOffset(slot) +
                        bitmapEntryOffsetInSector(slot);
  if (offset + sizeof(entry) > bitmapPartition_->size ||
      esp_partition_read(bitmapPartition_, offset, &entry, sizeof(entry)) !=
          ESP_OK ||
      entry.magic != kBitmapMagic || entry.reminderId != record.id ||
      entry.checksum != bitmapChecksum(entry.reminderId, entry.bitmap)) {
    return false;
  }

  memcpy(record.displayBitmap, entry.bitmap, kReminderBitmapSize);
  record.displayBitmapLength = kReminderBitmapSize;
  return true;
}

bool ReminderStore::saveDisplayBitmap(uint8_t slot, uint32_t reminderId,
                                      const uint8_t* displayBitmap) {
  if (bitmapPartition_ == nullptr || slot >= kMaxReminders ||
      reminderId == 0 || displayBitmap == nullptr) {
    return false;
  }

  const size_t sectorOffset = bitmapSectorOffset(slot);
  const size_t entryOffset = bitmapEntryOffsetInSector(slot);
  if (sectorOffset + kFlashSectorSize > bitmapPartition_->size ||
      entryOffset + sizeof(ReminderBitmapEntry) > kFlashSectorSize) {
    return false;
  }

  uint8_t sector[kFlashSectorSize];
  if (esp_partition_read(bitmapPartition_, sectorOffset, sector,
                         sizeof(sector)) != ESP_OK) {
    return false;
  }

  ReminderBitmapEntry entry;
  entry.magic = kBitmapMagic;
  entry.reminderId = reminderId;
  memcpy(entry.bitmap, displayBitmap, kReminderBitmapSize);
  entry.checksum = bitmapChecksum(entry.reminderId, entry.bitmap);
  memcpy(sector + entryOffset, &entry, sizeof(entry));

  if (esp_partition_erase_range(bitmapPartition_, sectorOffset,
                                kFlashSectorSize) != ESP_OK) {
    return false;
  }
  return esp_partition_write(bitmapPartition_, sectorOffset, sector,
                             sizeof(sector)) == ESP_OK;
}

int ReminderStore::findReusableSlot() const {
  if (records_ == nullptr) {
    return -1;
  }
  for (uint8_t index = 0; index < kMaxReminders; ++index) {
    if (!records_[index].active) {
      return index;
    }
  }
  for (uint8_t index = 0; index < kMaxReminders; ++index) {
    if (records_[index].completed) {
      return index;
    }
  }
  return -1;
}

ReminderRecord* ReminderStore::findById(uint32_t id) {
  if (records_ == nullptr) {
    return nullptr;
  }
  for (uint8_t index = 0; index < kMaxReminders; ++index) {
    if (records_[index].active && records_[index].id == id) {
      return &records_[index];
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
