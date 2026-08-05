#pragma once

#include <Arduino.h>
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

struct RadarDeskConfigResult {
  RadarCommandResult resolution;
  RadarCommandResult basicConfig;
  RadarCommandResult reboot;
};

struct RadarCalibrationStatus {
  bool received = false;
  bool success = false;
  bool running = false;
  uint16_t rawStatus = 0;
};

struct RadarSensitivityConfig {
  bool received = false;
  bool success = false;
  uint8_t gates[14] = {};
};

struct RadarTargetSnapshot {
  bool received = false;
  bool hasTarget = false;
  bool stateTarget = false;
  bool energyTarget = false;
  uint8_t targetConfidence = 0;
  uint32_t sequence = 0;
  uint8_t rawPayloadLength = 0;
  uint8_t rawPayload[16] = {};
  uint8_t targetState = 0;
  uint16_t targetDistanceCm = 0;
  uint8_t movingEnergy = 0;
  uint16_t movingDistanceCm = 0;
  uint8_t staticEnergy = 0;
  uint16_t staticDistanceCm = 0;
  uint32_t frameAgeMs = 0;
  uint32_t validFrameCount = 0;
  uint32_t invalidFrameCount = 0;
};

struct RadarParserSelfTestResult {
  bool normalFrameAccepted = false;
  bool staticTargetAccepted = false;
  bool clearFrameAccepted = false;
  bool corruptFrameRejected = false;

  bool passed() const {
    return normalFrameAccepted && staticTargetAccepted &&
           clearFrameAccepted && corruptFrameRejected;
  }
};

class RadarSensor {
 public:
  void begin();
  void update();
  RadarSnapshot read() const;
  RadarBasicConfig readBasicConfig();
  RadarResolutionConfig readResolutionConfig();
  bool applyNearDeskConfig();
  RadarDeskConfigResult applyDeskMode();
  RadarCommandResult applyResolution20cm();
  RadarCommandResult startBackgroundCalibration();
  RadarCalibrationStatus readBackgroundCalibrationStatus();
  RadarSensitivityConfig readMotionSensitivity();
  RadarSensitivityConfig readStaticSensitivity();
  RadarTargetSnapshot readTarget() const;
  RadarParserSelfTestResult parserSelfTest() const;
  void bridge(Stream& host);

 private:
  static constexpr size_t kReportBufferSize = 80;

  void consumeReportByte(uint8_t value);
  void parseReportPayload(const uint8_t* payload, size_t payloadLength);
  void resetReportParser();

  uint8_t reportBuffer_[kReportBufferSize] = {};
  size_t reportBufferLength_ = 0;
  size_t expectedReportLength_ = 0;
  RadarTargetSnapshot latestTarget_;
  unsigned long lastValidFrameMs_ = 0;
  uint32_t validFrameCount_ = 0;
  uint32_t invalidFrameCount_ = 0;
};

}  // namespace tongdou
