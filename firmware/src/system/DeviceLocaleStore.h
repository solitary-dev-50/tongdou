#pragma once

#include <Arduino.h>

namespace tongdou {

class DeviceLocaleStore {
 public:
  void begin();
  String locale() const;
  bool save(const String& locale);
  bool ready() const;

  static const char* defaultLocale();
  static bool supported(const String& locale);

 private:
  bool ready_ = false;
  String locale_;
};

}  // namespace tongdou
