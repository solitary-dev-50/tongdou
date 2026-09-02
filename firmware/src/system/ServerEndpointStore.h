#pragma once

#include <Arduino.h>

namespace tongdou {

class ServerEndpointStore {
 public:
  void begin();
  String otaUrl() const;
  bool saveOtaUrl(const String& otaUrl);

 private:
  bool ready_ = false;
};

}  // namespace tongdou
