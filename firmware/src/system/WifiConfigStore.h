#pragma once

#include <Arduino.h>

namespace tongdou {

struct WifiCredentials {
  String ssid;
  String password;
  String serverUrl;
};

class WifiConfigStore {
 public:
  static constexpr uint8_t kMaxNetworks = 8;

  void begin(const String& currentServerUrl);
  uint8_t count() const;
  uint8_t preferredIndex() const;
  bool loadAt(uint8_t index, WifiCredentials& credentials) const;
  bool save(const WifiCredentials& credentials, uint8_t& index);
  bool remove(const String& ssid);
  void markConnected(uint8_t index);
  void clear();

 private:
  bool ready_ = false;
};

}  // namespace tongdou
