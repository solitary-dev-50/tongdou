#pragma once

#include <Arduino.h>

namespace tongdou {

struct WifiCredentials {
  String ssid;
  String password;
};

class WifiConfigStore {
 public:
  void begin();
  bool load(WifiCredentials& credentials);
  void save(const WifiCredentials& credentials);
  void clear();

 private:
  bool ready_ = false;
};

}  // namespace tongdou
