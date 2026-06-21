#pragma once

namespace tongdou {

struct RadarSnapshot {
  bool occupied = false;
  bool rxLevelHigh = false;
};

class RadarSensor {
 public:
  void begin();
  RadarSnapshot read() const;
};

}  // namespace tongdou
