#pragma once

#include <Arduino.h>
#include <esp_partition.h>

namespace tongdou {

struct LocaleResourceFile {
  uint32_t offset = 0;
  uint32_t size = 0;
};

class LocaleResourceStore {
 public:
  void begin();

  bool ready() const;
  const String& locale() const;
  uint32_t version() const;
  uint32_t capacity() const;
  const String& lastError() const;

  bool matchesLocale(const String& locale) const;
  bool installPack(const uint8_t* data, size_t size,
                   const String& expectedLocale);
  bool findFile(const char* name, LocaleResourceFile& file) const;
  bool readFile(const LocaleResourceFile& file, uint32_t offset, void* output,
                size_t size) const;

 private:
  static constexpr uint16_t kMaxFiles = 32;
  static constexpr size_t kFileNameLength = 32;

#pragma pack(push, 1)
  struct PackHeader {
    char magic[8];
    uint16_t formatVersion;
    char locale[8];
    uint16_t fileCount;
    uint32_t resourceVersion;
    uint32_t directoryOffset;
    uint32_t payloadOffset;
    uint32_t payloadSize;
    uint32_t totalSize;
    uint8_t contentSha256[32];
  };

  struct PackEntry {
    char name[kFileNameLength];
    uint32_t offset;
    uint32_t size;
  };
#pragma pack(pop)

  static_assert(sizeof(PackHeader) == 72, "locale pack header layout changed");
  static_assert(sizeof(PackEntry) == 40, "locale pack entry layout changed");

  bool validateHeader(const PackHeader& header, uint32_t availableSize,
                      String& error) const;
  bool validateDirectoryEntries(const PackHeader& header,
                                const PackEntry* entries,
                                String& error) const;
  bool validateMemoryPack(const uint8_t* data, size_t size,
                          const String& expectedLocale,
                          String& error) const;
  bool loadDirectory(const PackHeader& header);
  bool validateContent(const PackHeader& header);
  void fail(const String& error);

  const esp_partition_t* partition_ = nullptr;
  PackEntry entries_[kMaxFiles] = {};
  uint16_t fileCount_ = 0;
  String locale_;
  uint32_t version_ = 0;
  bool ready_ = false;
  String lastError_;
};

}  // namespace tongdou
