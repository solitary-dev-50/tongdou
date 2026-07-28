#pragma once

#include <Arduino.h>
#include <time.h>

namespace tongdou {

class TimeService {
 public:
  void begin();
  void update();
  void configureNetworkTime();
  bool setEpoch(uint32_t epochSeconds);
  bool ready() const;
  time_t now() const;
  String isoNow() const;

 private:
  bool configuredNetworkTime_ = false;
};

}  // namespace tongdou
