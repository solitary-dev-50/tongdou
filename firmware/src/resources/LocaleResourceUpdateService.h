#pragma once

#include <Arduino.h>

#include "resources/LocaleResourceStore.h"

namespace tongdou {

struct LocaleResourceUpdateResult {
  bool ok = false;
  size_t downloadedBytes = 0;
  String error;
};

class LocaleResourceUpdateService {
 public:
  explicit LocaleResourceUpdateService(LocaleResourceStore& store);

  void setEndpoint(const String& baseUrl, uint16_t formatVersion);
  LocaleResourceUpdateResult downloadAndInstall(const String& locale);

  const String& baseUrl() const;
  uint16_t formatVersion() const;

 private:
  LocaleResourceUpdateResult fail(const String& error, size_t downloadedBytes = 0);

  LocaleResourceStore& store_;
  String baseUrl_;
  uint16_t formatVersion_ = 0;
};

}  // namespace tongdou
