#include "resources/LocaleResourceStore.h"

#include <cstring>

#include <mbedtls/sha256.h>

namespace tongdou {
namespace {

constexpr char kPartitionLabel[] = "locale";
constexpr char kPackMagic[] = "TDLPACK1";
constexpr uint16_t kPackFormatVersion = 1;
constexpr size_t kHashChunkSize = 1024;
constexpr size_t kFlashWriteChunkSize = 1024;
constexpr const char* kRequiredFiles[] = {
    "welcome.ogg",          "wifi_config.ogg",      "upgrade.ogg",
    "activation_code.ogg", "activation_error.ogg", "pin_error.ogg",
    "offline_notice.ogg",
    "digit_0.ogg",          "digit_1.ogg",           "digit_2.ogg",
    "digit_3.ogg",          "digit_4.ogg",           "digit_5.ogg",
    "digit_6.ogg",          "digit_7.ogg",           "digit_8.ogg",
    "digit_9.ogg",
};

}  // namespace

void LocaleResourceStore::begin() {
  ready_ = false;
  lastError_ = "";
  locale_ = "";
  version_ = 0;
  fileCount_ = 0;
  memset(entries_, 0, sizeof(entries_));

  partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                        ESP_PARTITION_SUBTYPE_ANY,
                                        kPartitionLabel);
  if (partition_ == nullptr) {
    fail("locale_partition_missing");
    return;
  }

  PackHeader header = {};
  if (esp_partition_read(partition_, 0, &header, sizeof(header)) != ESP_OK) {
    fail("locale_header_read_failed");
    return;
  }
  String error;
  if (!validateHeader(header, partition_->size, error)) {
    fail(error);
    return;
  }
  if (!loadDirectory(header) || !validateContent(header)) {
    return;
  }

  locale_ = String(header.locale);
  version_ = header.resourceVersion;
  ready_ = true;
  lastError_ = "";
}

bool LocaleResourceStore::ready() const {
  return ready_;
}

const String& LocaleResourceStore::locale() const {
  return locale_;
}

uint32_t LocaleResourceStore::version() const {
  return version_;
}

uint32_t LocaleResourceStore::capacity() const {
  return partition_ == nullptr ? 0 : partition_->size;
}

const String& LocaleResourceStore::lastError() const {
  return lastError_;
}

bool LocaleResourceStore::matchesLocale(const String& locale) const {
  return ready_ && locale_ == locale;
}

bool LocaleResourceStore::installPack(const uint8_t* data, size_t size,
                                      const String& expectedLocale) {
  if (partition_ == nullptr) {
    lastError_ = "locale_partition_missing";
    return false;
  }

  String error;
  if (!validateMemoryPack(data, size, expectedLocale, error)) {
    lastError_ = error;
    return false;
  }

  ready_ = false;
  if (esp_partition_erase_range(partition_, 0, partition_->size) != ESP_OK) {
    fail("locale_partition_erase_failed");
    return false;
  }

  uint8_t writeBuffer[kFlashWriteChunkSize];
  size_t offset = 0;
  while (offset < size) {
    const size_t chunkSize = min(sizeof(writeBuffer), size - offset);
    memcpy(writeBuffer, data + offset, chunkSize);
    if (esp_partition_write(partition_, offset, writeBuffer, chunkSize) !=
        ESP_OK) {
      fail("locale_partition_write_failed");
      return false;
    }
    offset += chunkSize;
  }

  begin();
  if (!ready_ || locale_ != expectedLocale) {
    if (lastError_.length() == 0) {
      fail("locale_partition_verify_failed");
    }
    return false;
  }
  return true;
}

bool LocaleResourceStore::findFile(const char* name,
                                   LocaleResourceFile& file) const {
  if (!ready_ || name == nullptr || name[0] == '\0') {
    return false;
  }

  for (uint16_t i = 0; i < fileCount_; ++i) {
    if (strncmp(entries_[i].name, name, kFileNameLength) == 0) {
      file.offset = entries_[i].offset;
      file.size = entries_[i].size;
      return true;
    }
  }
  return false;
}

bool LocaleResourceStore::readFile(const LocaleResourceFile& file,
                                   uint32_t offset, void* output,
                                   size_t size) const {
  if (!ready_ || partition_ == nullptr || output == nullptr || size == 0 ||
      offset > file.size || size > file.size - offset) {
    return false;
  }

  return esp_partition_read(partition_, file.offset + offset, output, size) ==
         ESP_OK;
}

bool LocaleResourceStore::validateHeader(const PackHeader& header,
                                         uint32_t availableSize,
                                         String& error) const {
  if (memcmp(header.magic, kPackMagic, sizeof(header.magic)) != 0) {
    error = "locale_pack_missing";
    return false;
  }
  if (header.formatVersion != kPackFormatVersion) {
    error = "locale_pack_version_unsupported";
    return false;
  }
  if (header.locale[0] == '\0' || header.locale[sizeof(header.locale) - 1] != '\0') {
    error = "locale_pack_locale_invalid";
    return false;
  }
  if (header.fileCount == 0 || header.fileCount > kMaxFiles) {
    error = "locale_pack_file_count_invalid";
    return false;
  }
  if (header.directoryOffset != sizeof(PackHeader) ||
      header.payloadOffset !=
          sizeof(PackHeader) + header.fileCount * sizeof(PackEntry) ||
      header.payloadSize == 0 ||
      header.totalSize != header.payloadOffset + header.payloadSize ||
      header.totalSize > availableSize) {
    error = "locale_pack_bounds_invalid";
    return false;
  }
  return true;
}

bool LocaleResourceStore::validateDirectoryEntries(
    const PackHeader& header, const PackEntry* entries, String& error) const {
  if (entries == nullptr) {
    error = "locale_directory_missing";
    return false;
  }

  for (uint16_t i = 0; i < header.fileCount; ++i) {
    const PackEntry& entry = entries[i];
    if (entry.name[0] == '\0' || entry.name[kFileNameLength - 1] != '\0' ||
        entry.size == 0 || entry.offset < header.payloadOffset ||
        entry.offset > header.totalSize ||
        entry.size > header.totalSize - entry.offset) {
      error = "locale_directory_entry_invalid";
      return false;
    }
  }

  for (const char* required : kRequiredFiles) {
    bool found = false;
    for (uint16_t i = 0; i < header.fileCount; ++i) {
      if (strncmp(entries[i].name, required, kFileNameLength) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      error = String("locale_required_file_missing:") + required;
      return false;
    }
  }
  return true;
}

bool LocaleResourceStore::validateMemoryPack(
    const uint8_t* data, size_t size, const String& expectedLocale,
    String& error) const {
  if (data == nullptr || size < sizeof(PackHeader) || size > capacity()) {
    error = "locale_download_size_invalid";
    return false;
  }

  PackHeader header = {};
  memcpy(&header, data, sizeof(header));
  if (header.totalSize != size ||
      !validateHeader(header, static_cast<uint32_t>(size), error)) {
    if (error.length() == 0) {
      error = "locale_download_total_size_mismatch";
    }
    return false;
  }
  if (String(header.locale) != expectedLocale) {
    error = "locale_download_locale_mismatch";
    return false;
  }

  PackEntry entries[kMaxFiles] = {};
  const size_t directorySize = header.fileCount * sizeof(PackEntry);
  memcpy(entries, data + header.directoryOffset, directorySize);
  if (!validateDirectoryEntries(header, entries, error)) {
    return false;
  }

  uint8_t digest[32] = {};
  const size_t contentSize = header.totalSize - header.directoryOffset;
  if (mbedtls_sha256_ret(data + header.directoryOffset, contentSize, digest,
                         0) != 0) {
    error = "locale_download_hash_calculation_failed";
    return false;
  }
  if (memcmp(digest, header.contentSha256, sizeof(digest)) != 0) {
    error = "locale_download_hash_mismatch";
    return false;
  }
  return true;
}

bool LocaleResourceStore::loadDirectory(const PackHeader& header) {
  const size_t directorySize = header.fileCount * sizeof(PackEntry);
  if (esp_partition_read(partition_, header.directoryOffset, entries_,
                         directorySize) != ESP_OK) {
    fail("locale_directory_read_failed");
    return false;
  }

  String error;
  if (!validateDirectoryEntries(header, entries_, error)) {
    fail(error);
    return false;
  }

  fileCount_ = header.fileCount;
  return true;
}

bool LocaleResourceStore::validateContent(const PackHeader& header) {
  mbedtls_sha256_context context;
  mbedtls_sha256_init(&context);
  if (mbedtls_sha256_starts_ret(&context, 0) != 0) {
    mbedtls_sha256_free(&context);
    fail("locale_hash_start_failed");
    return false;
  }

  uint8_t buffer[kHashChunkSize];
  uint32_t offset = 0;
  bool ok = true;
  const uint32_t contentSize = header.totalSize - header.directoryOffset;
  while (offset < contentSize) {
    const size_t chunkSize =
        min<size_t>(sizeof(buffer), contentSize - offset);
    if (esp_partition_read(partition_, header.directoryOffset + offset, buffer,
                           chunkSize) != ESP_OK ||
        mbedtls_sha256_update_ret(&context, buffer, chunkSize) != 0) {
      ok = false;
      break;
    }
    offset += static_cast<uint32_t>(chunkSize);
  }

  uint8_t digest[32] = {};
  if (!ok || mbedtls_sha256_finish_ret(&context, digest) != 0) {
    mbedtls_sha256_free(&context);
    fail("locale_hash_calculation_failed");
    return false;
  }
  mbedtls_sha256_free(&context);

  if (memcmp(digest, header.contentSha256, sizeof(digest)) != 0) {
    fail("locale_hash_mismatch");
    return false;
  }
  return true;
}

void LocaleResourceStore::fail(const String& error) {
  ready_ = false;
  lastError_ = error;
}

}  // namespace tongdou
