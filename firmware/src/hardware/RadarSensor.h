#pragma once

#include <stdint.h>

namespace tongdou {

struct RadarSnapshot {
  bool occupied = false;
  bool rxLevelHigh = false;
};

struct RadarBasicConfig {
  bool received = false;
  bool success = false;
  uint8_t minGate = 0;
  uint8_t maxGate = 0;
  uint16_t absenceHoldSeconds = 0;
  uint8_t outputPolarity = 0;
};

struct RadarResolutionConfig {
  bool received = false;
  bool success = false;
  uint8_t resolutionValue = 0;
  uint8_t gateCentimeters = 75;
};

struct RadarCommandResult {
  bool received = false;
  bool success = false;
};

struct RadarTargetSnapshot {
  bool received = false;
  bool hasTarget = false;
  uint8_t rawPayloadLength = 0;
  uint8_t rawPayload[16] = {};
  uint8_t targetState = 0;
  uint16_t targetDistanceCm = 0;
  uint8_t movingEnergy = 0;
  uint16_t movingDistanceCm = 0;
  uint8_t staticEnergy = 0;
  uint16_t staticDistanceCm = 0;
};

class RadarSensor {
 public:
  void begin();
  RadarSnapshot read() const;
  RadarBasicConfig readBasicConfig();
  RadarResolutionConfig readResolutionConfig();
  bool applyNearDeskConfig();
  RadarCommandResult applyResolution20cm();
  RadarCommandResult startBackgroundCalibration();
  RadarTargetSnapshot readTarget();
};

}  // namespace tongdou
