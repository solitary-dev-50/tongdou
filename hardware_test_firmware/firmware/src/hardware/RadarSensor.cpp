#include "hardware/RadarSensor.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

HardwareSerial radarSerial(1);

constexpr uint32_t kRadarBaud = 115200;
constexpr uint16_t kCommandTimeoutMs = 300;
constexpr size_t kMaxFrameSize = 80;
constexpr uint8_t kTargetConfirmFrames = 2;
constexpr uint8_t kTargetClearFrames = 2;
constexpr uint16_t kTargetFrameStaleMs = 1000;
constexpr uint16_t kTargetMinDistanceCm = 20;
constexpr uint16_t kTargetMaxDistanceCm = 260;
constexpr uint8_t kMovingEnergyThreshold = 20;
constexpr uint8_t kStaticEnergyThreshold = 25;
constexpr uint8_t kReportHeader[] = {0xF4, 0xF3, 0xF2, 0xF1};
constexpr uint8_t kReportFooter[] = {0xF8, 0xF7, 0xF6, 0xF5};

constexpr uint16_t kEnableConfigAck = 0x01FF;
constexpr uint16_t kEndConfigAck = 0x01FE;
constexpr uint16_t kReadResolutionAck = 0x0111;
constexpr uint16_t kSetResolutionAck = 0x0101;
constexpr uint16_t kReadBasicConfigAck = 0x0112;
constexpr uint16_t kReadMotionSensitivityAck = 0x0113;
constexpr uint16_t kReadStaticSensitivityAck = 0x0114;
constexpr uint16_t kSetBasicConfigAck = 0x0102;
constexpr uint16_t kBackgroundCalibrationAck = 0x010B;
constexpr uint16_t kReadBackgroundCalibrationStatusAck = 0x011B;
constexpr uint16_t kRebootAck = 0x01A3;

constexpr uint8_t kEnableConfigCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kEndConfigCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kReadResolutionCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x11, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kSetResolution20cmCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kReadBasicConfigCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x12, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kReadMotionSensitivityCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x13, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kReadStaticSensitivityCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x14, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kNearDeskConfigCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x07, 0x00, 0x02, 0x00, 0x01, 0x04, 0x05,
    0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kDeskBasicConfigCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x07, 0x00, 0x02, 0x00, 0x01, 0x0D, 0x05,
    0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kBackgroundCalibrationCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x0B, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kReadBackgroundCalibrationStatusCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x1B, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kRebootCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA3, 0x00, 0x04, 0x03, 0x02, 0x01};

void clearRadarInput() {
  while (radarSerial.available() > 0) {
    radarSerial.read();
  }
}

void sendRadarCommand(const uint8_t* command, size_t length) {
  radarSerial.write(command, length);
  radarSerial.flush();
}

bool readRadarFrame(uint16_t expectedCommand, uint8_t* payload, size_t payloadCapacity,
                    size_t& payloadLength, uint16_t timeoutMs) {
  uint8_t buffer[kMaxFrameSize] = {};
  size_t count = 0;
  const unsigned long startMs = millis();

  while (millis() - startMs < timeoutMs) {
    while (radarSerial.available() > 0) {
      if (count < sizeof(buffer)) {
        buffer[count++] = static_cast<uint8_t>(radarSerial.read());
      } else {
        memmove(buffer, buffer + 1, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = static_cast<uint8_t>(radarSerial.read());
      }

      while (count >= 4 &&
             !(buffer[0] == 0xFD && buffer[1] == 0xFC && buffer[2] == 0xFB &&
               buffer[3] == 0xFA)) {
        memmove(buffer, buffer + 1, count - 1);
        --count;
      }

      if (count < 8) {
        continue;
      }

      const uint16_t frameLength = static_cast<uint16_t>(buffer[4]) |
                                   (static_cast<uint16_t>(buffer[5]) << 8);
      const size_t totalLength = 4 + 2 + frameLength + 4;
      if (totalLength > sizeof(buffer)) {
        count = 0;
        continue;
      }
      if (count < totalLength) {
        continue;
      }

      const bool footerOk = buffer[totalLength - 4] == 0x04 &&
                            buffer[totalLength - 3] == 0x03 &&
                            buffer[totalLength - 2] == 0x02 &&
                            buffer[totalLength - 1] == 0x01;
      const uint16_t command = static_cast<uint16_t>(buffer[6]) |
                               (static_cast<uint16_t>(buffer[7]) << 8);
      if (footerOk && command == expectedCommand) {
        payloadLength = frameLength >= 2 ? frameLength - 2 : 0;
        const size_t copyLength = min(payloadLength, payloadCapacity);
        memcpy(payload, buffer + 8, copyLength);
        return true;
      }

      memmove(buffer, buffer + totalLength, count - totalLength);
      count -= totalLength;
    }
    delay(2);
  }

  return false;
}

bool sendAndWaitAck(const uint8_t* command, size_t length, uint16_t ackCommand) {
  uint8_t payload[16] = {};
  size_t payloadLength = 0;
  sendRadarCommand(command, length);
  return readRadarFrame(ackCommand, payload, sizeof(payload), payloadLength, kCommandTimeoutMs);
}

RadarCommandResult sendAndReadAckStatus(const uint8_t* command, size_t length,
                                        uint16_t ackCommand) {
  RadarCommandResult result;
  uint8_t payload[16] = {};
  size_t payloadLength = 0;
  sendRadarCommand(command, length);
  result.received =
      readRadarFrame(ackCommand, payload, sizeof(payload), payloadLength, kCommandTimeoutMs);
  if (result.received && payloadLength >= 2) {
    const uint16_t ackStatus = static_cast<uint16_t>(payload[0]) |
                               (static_cast<uint16_t>(payload[1]) << 8);
    result.success = (ackStatus == 0);
  }
  return result;
}

uint8_t gateCentimetersFromResolution(uint8_t value) {
  if (value == 1) {
    return 50;
  }
  if (value == 3) {
    return 20;
  }
  return 75;
}

bool distanceInTargetRange(uint16_t distanceCm) {
  return distanceCm >= kTargetMinDistanceCm && distanceCm <= kTargetMaxDistanceCm;
}

}  // namespace

void RadarSensor::begin() {
  pinMode(pins::RADAR_OUT, INPUT);
  pinMode(pins::RADAR_RX_FROM_MODULE_TX, INPUT);
  radarSerial.begin(kRadarBaud, SERIAL_8N1, static_cast<int>(pins::RADAR_RX_FROM_MODULE_TX),
                    static_cast<int>(pins::RADAR_TX_TO_MODULE_RX));
  resetReportParser();
  latestTarget_ = {};
  lastValidFrameMs_ = 0;
  validFrameCount_ = 0;
  invalidFrameCount_ = 0;
  presentStreak_ = 0;
  clearStreak_ = 0;
  stableHasTarget_ = false;
  clearRadarInput();
}

void RadarSensor::update() {
  size_t byteBudget = 256;
  while (radarSerial.available() > 0 && byteBudget > 0) {
    consumeReportByte(static_cast<uint8_t>(radarSerial.read()));
    --byteBudget;
  }
}

void RadarSensor::consumeReportByte(uint8_t value) {
  if (reportBufferLength_ < sizeof(kReportHeader)) {
    if (value == kReportHeader[reportBufferLength_]) {
      reportBuffer_[reportBufferLength_++] = value;
      return;
    }

    reportBufferLength_ = 0;
    expectedReportLength_ = 0;
    if (value == kReportHeader[0]) {
      reportBuffer_[reportBufferLength_++] = value;
    }
    return;
  }

  if (reportBufferLength_ >= sizeof(reportBuffer_)) {
    ++invalidFrameCount_;
    resetReportParser();
    if (value == kReportHeader[0]) {
      reportBuffer_[reportBufferLength_++] = value;
    }
    return;
  }

  reportBuffer_[reportBufferLength_++] = value;
  if (reportBufferLength_ == 6) {
    const uint16_t payloadLength = static_cast<uint16_t>(reportBuffer_[4]) |
                                   (static_cast<uint16_t>(reportBuffer_[5]) << 8);
    expectedReportLength_ = 4 + 2 + payloadLength + 4;
    if (payloadLength < 11 || expectedReportLength_ > sizeof(reportBuffer_)) {
      ++invalidFrameCount_;
      resetReportParser();
    }
    return;
  }

  if (expectedReportLength_ == 0 || reportBufferLength_ < expectedReportLength_) {
    return;
  }

  const size_t footerOffset = expectedReportLength_ - sizeof(kReportFooter);
  const bool footerOk =
      memcmp(reportBuffer_ + footerOffset, kReportFooter, sizeof(kReportFooter)) == 0;
  if (footerOk) {
    parseReportPayload(reportBuffer_ + 6, expectedReportLength_ - 10);
  } else {
    ++invalidFrameCount_;
  }
  resetReportParser();
}

void RadarSensor::parseReportPayload(const uint8_t* payload, size_t payloadLength) {
  if (payload == nullptr || payloadLength < 11 ||
      (payload[0] != 0x01 && payload[0] != 0x02) ||
      payload[1] != 0xAA || payload[payloadLength - 2] != 0x55 ||
      payload[payloadLength - 1] != 0x00) {
    ++invalidFrameCount_;
    return;
  }

  RadarTargetSnapshot snapshot;
  snapshot.received = true;
  snapshot.sequence = ++validFrameCount_;
  snapshot.validFrameCount = validFrameCount_;
  snapshot.invalidFrameCount = invalidFrameCount_;
  snapshot.targetState = payload[2];
  snapshot.movingDistanceCm = static_cast<uint16_t>(payload[3]) |
                              (static_cast<uint16_t>(payload[4]) << 8);
  snapshot.movingEnergy = payload[5];
  snapshot.staticDistanceCm = static_cast<uint16_t>(payload[6]) |
                              (static_cast<uint16_t>(payload[7]) << 8);
  snapshot.staticEnergy = payload[8];

  const size_t rawLength = min(payloadLength, sizeof(snapshot.rawPayload));
  snapshot.rawPayloadLength = static_cast<uint8_t>(rawLength);
  memcpy(snapshot.rawPayload, payload, rawLength);

  const bool rawTarget = snapshot.targetState >= 1 && snapshot.targetState <= 3;
  snapshot.stateTarget = rawTarget;

  const bool movingEnergyTarget =
      distanceInTargetRange(snapshot.movingDistanceCm) &&
      snapshot.movingEnergy >= kMovingEnergyThreshold;
  const bool staticEnergyTarget =
      distanceInTargetRange(snapshot.staticDistanceCm) &&
      snapshot.staticEnergy >= kStaticEnergyThreshold;
  snapshot.energyTarget = movingEnergyTarget || staticEnergyTarget;

  if (rawTarget) {
    presentStreak_ = min<uint8_t>(presentStreak_ + 1, kTargetConfirmFrames);
    clearStreak_ = 0;
    if (presentStreak_ >= kTargetConfirmFrames) {
      stableHasTarget_ = true;
    }
  } else if (snapshot.targetState == 0) {
    clearStreak_ = min<uint8_t>(clearStreak_ + 1, kTargetClearFrames);
    presentStreak_ = 0;
    if (clearStreak_ >= kTargetClearFrames) {
      stableHasTarget_ = false;
    }
  }

  snapshot.targetConfidence =
      stableHasTarget_ ? kTargetConfirmFrames : presentStreak_;
  snapshot.hasTarget = stableHasTarget_;

  const bool hasMoving = (snapshot.targetState & 0x01) != 0;
  const bool hasStatic = (snapshot.targetState & 0x02) != 0;
  if (hasMoving && hasStatic) {
    if (snapshot.movingDistanceCm == 0) {
      snapshot.targetDistanceCm = snapshot.staticDistanceCm;
    } else if (snapshot.staticDistanceCm == 0) {
      snapshot.targetDistanceCm = snapshot.movingDistanceCm;
    } else {
      snapshot.targetDistanceCm =
          min(snapshot.movingDistanceCm, snapshot.staticDistanceCm);
    }
  } else if (hasMoving) {
    snapshot.targetDistanceCm = snapshot.movingDistanceCm;
  } else if (hasStatic) {
    snapshot.targetDistanceCm = snapshot.staticDistanceCm;
  }

  lastValidFrameMs_ = millis();
  latestTarget_ = snapshot;
}

void RadarSensor::resetReportParser() {
  reportBufferLength_ = 0;
  expectedReportLength_ = 0;
}

RadarSnapshot RadarSensor::read() const {
  RadarSnapshot snapshot;
  snapshot.occupied = digitalRead(pins::RADAR_OUT) == HIGH;
  snapshot.rxLevelHigh = digitalRead(pins::RADAR_RX_FROM_MODULE_TX) == HIGH;
  return snapshot;
}

RadarBasicConfig RadarSensor::readBasicConfig() {
  RadarBasicConfig config;
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return config;
  }

  uint8_t payload[16] = {};
  size_t payloadLength = 0;
  sendRadarCommand(kReadBasicConfigCommand, sizeof(kReadBasicConfigCommand));
  config.received = readRadarFrame(kReadBasicConfigAck, payload, sizeof(payload), payloadLength,
                                   kCommandTimeoutMs);
  if (config.received && payloadLength >= 7) {
    const uint16_t ackStatus = static_cast<uint16_t>(payload[0]) |
                               (static_cast<uint16_t>(payload[1]) << 8);
    config.success = (ackStatus == 0);
    config.minGate = payload[2];
    config.maxGate = payload[3];
    config.absenceHoldSeconds = static_cast<uint16_t>(payload[4]) |
                                (static_cast<uint16_t>(payload[5]) << 8);
    config.outputPolarity = payload[6];
  }

  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return config;
}

RadarResolutionConfig RadarSensor::readResolutionConfig() {
  RadarResolutionConfig config;
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return config;
  }

  uint8_t payload[16] = {};
  size_t payloadLength = 0;
  sendRadarCommand(kReadResolutionCommand, sizeof(kReadResolutionCommand));
  config.received = readRadarFrame(kReadResolutionAck, payload, sizeof(payload), payloadLength,
                                   kCommandTimeoutMs);
  if (config.received && payloadLength >= 3) {
    const uint16_t ackStatus = static_cast<uint16_t>(payload[0]) |
                               (static_cast<uint16_t>(payload[1]) << 8);
    config.success = (ackStatus == 0);
    config.resolutionValue = payload[2];
    config.gateCentimeters = gateCentimetersFromResolution(config.resolutionValue);
  }

  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return config;
}

bool RadarSensor::applyNearDeskConfig() {
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return false;
  }

  const bool configured =
      sendAndWaitAck(kNearDeskConfigCommand, sizeof(kNearDeskConfigCommand), kSetBasicConfigAck);
  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return configured;
}

RadarDeskConfigResult RadarSensor::applyDeskMode() {
  RadarDeskConfigResult result;
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return result;
  }

  result.resolution =
      sendAndReadAckStatus(kSetResolution20cmCommand, sizeof(kSetResolution20cmCommand),
                           kSetResolutionAck);
  if (result.resolution.success) {
    result.reboot = sendAndReadAckStatus(kRebootCommand, sizeof(kRebootCommand), kRebootAck);
  } else {
    sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
    return result;
  }

  if (!result.reboot.success) {
    return result;
  }

  delay(2500);
  resetReportParser();
  clearRadarInput();
  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return result;
  }

  result.basicConfig =
      sendAndReadAckStatus(kDeskBasicConfigCommand, sizeof(kDeskBasicConfigCommand),
                           kSetBasicConfigAck);
  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return result;
}

RadarCommandResult RadarSensor::applyResolution20cm() {
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return {};
  }

  RadarCommandResult result =
      sendAndReadAckStatus(kSetResolution20cmCommand, sizeof(kSetResolution20cmCommand),
                           kSetResolutionAck);
  if (result.success) {
    sendAndReadAckStatus(kRebootCommand, sizeof(kRebootCommand), kRebootAck);
  } else {
    sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  }
  return result;
}

RadarCommandResult RadarSensor::startBackgroundCalibration() {
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return {};
  }

  RadarCommandResult result =
      sendAndReadAckStatus(kBackgroundCalibrationCommand, sizeof(kBackgroundCalibrationCommand),
                           kBackgroundCalibrationAck);
  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return result;
}

RadarCalibrationStatus RadarSensor::readBackgroundCalibrationStatus() {
  RadarCalibrationStatus status;
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return status;
  }

  uint8_t payload[16] = {};
  size_t payloadLength = 0;
  sendRadarCommand(kReadBackgroundCalibrationStatusCommand,
                   sizeof(kReadBackgroundCalibrationStatusCommand));
  status.received = readRadarFrame(kReadBackgroundCalibrationStatusAck, payload, sizeof(payload),
                                   payloadLength, kCommandTimeoutMs);
  if (status.received && payloadLength >= 4) {
    const uint16_t ackStatus = static_cast<uint16_t>(payload[0]) |
                               (static_cast<uint16_t>(payload[1]) << 8);
    status.rawStatus = static_cast<uint16_t>(payload[2]) |
                       (static_cast<uint16_t>(payload[3]) << 8);
    status.success = (ackStatus == 0);
    status.running = status.rawStatus == 1;
  }

  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return status;
}

RadarSensitivityConfig RadarSensor::readMotionSensitivity() {
  RadarSensitivityConfig config;
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return config;
  }

  uint8_t payload[18] = {};
  size_t payloadLength = 0;
  sendRadarCommand(kReadMotionSensitivityCommand, sizeof(kReadMotionSensitivityCommand));
  config.received =
      readRadarFrame(kReadMotionSensitivityAck, payload, sizeof(payload), payloadLength,
                     kCommandTimeoutMs);
  if (config.received && payloadLength >= 16) {
    const uint16_t ackStatus = static_cast<uint16_t>(payload[0]) |
                               (static_cast<uint16_t>(payload[1]) << 8);
    config.success = (ackStatus == 0);
    memcpy(config.gates, payload + 2, sizeof(config.gates));
  }

  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return config;
}

RadarSensitivityConfig RadarSensor::readStaticSensitivity() {
  RadarSensitivityConfig config;
  resetReportParser();
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return config;
  }

  uint8_t payload[18] = {};
  size_t payloadLength = 0;
  sendRadarCommand(kReadStaticSensitivityCommand, sizeof(kReadStaticSensitivityCommand));
  config.received =
      readRadarFrame(kReadStaticSensitivityAck, payload, sizeof(payload), payloadLength,
                     kCommandTimeoutMs);
  if (config.received && payloadLength >= 16) {
    const uint16_t ackStatus = static_cast<uint16_t>(payload[0]) |
                               (static_cast<uint16_t>(payload[1]) << 8);
    config.success = (ackStatus == 0);
    memcpy(config.gates, payload + 2, sizeof(config.gates));
  }

  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return config;
}

RadarTargetSnapshot RadarSensor::readTarget() const {
  RadarTargetSnapshot snapshot = latestTarget_;
  snapshot.validFrameCount = validFrameCount_;
  snapshot.invalidFrameCount = invalidFrameCount_;

  if (lastValidFrameMs_ == 0) {
    snapshot.received = false;
    snapshot.hasTarget = false;
    snapshot.frameAgeMs = 0;
    return snapshot;
  }

  snapshot.frameAgeMs = millis() - lastValidFrameMs_;
  snapshot.received = snapshot.frameAgeMs <= kTargetFrameStaleMs;
  if (!snapshot.received) {
    snapshot.hasTarget = false;
  }
  return snapshot;
}

RadarParserSelfTestResult RadarSensor::parserSelfTest() const {
  constexpr uint8_t kOfficialStaticTargetFrame[] = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x0B, 0x00, 0x02, 0xAA, 0x02, 0x51, 0x00,
      0x00, 0x00, 0x00, 0x3B, 0x55, 0x00, 0xF8, 0xF7, 0xF6, 0xF5};
  constexpr uint8_t kNoTargetFrame[] = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x0B, 0x00, 0x02, 0xAA, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x55, 0x00, 0xF8, 0xF7, 0xF6, 0xF5};

  RadarSensor parser;
  for (uint8_t repeat = 0; repeat < kTargetConfirmFrames; ++repeat) {
    for (uint8_t value : kOfficialStaticTargetFrame) {
      parser.consumeReportByte(value);
    }
  }

  RadarParserSelfTestResult result;
  RadarTargetSnapshot snapshot = parser.readTarget();
  result.normalFrameAccepted =
      snapshot.received && snapshot.validFrameCount == kTargetConfirmFrames;
  result.staticTargetAccepted =
      snapshot.targetState == 2 && snapshot.stateTarget && snapshot.hasTarget;

  for (uint8_t repeat = 0; repeat < kTargetClearFrames; ++repeat) {
    for (uint8_t value : kNoTargetFrame) {
      parser.consumeReportByte(value);
    }
  }
  snapshot = parser.readTarget();
  result.clearFrameAccepted =
      snapshot.received && snapshot.targetState == 0 && !snapshot.hasTarget;

  uint8_t corruptFrame[sizeof(kOfficialStaticTargetFrame)] = {};
  memcpy(corruptFrame, kOfficialStaticTargetFrame, sizeof(corruptFrame));
  corruptFrame[sizeof(corruptFrame) - 1] = 0x00;
  const uint32_t validBefore = parser.validFrameCount_;
  const uint32_t invalidBefore = parser.invalidFrameCount_;
  for (uint8_t value : corruptFrame) {
    parser.consumeReportByte(value);
  }
  result.corruptFrameRejected =
      parser.validFrameCount_ == validBefore &&
      parser.invalidFrameCount_ == invalidBefore + 1;
  return result;
}

void RadarSensor::bridge(Stream& host) {
  resetReportParser();
  clearRadarInput();

  while (true) {
    while (host.available() > 0) {
      radarSerial.write(static_cast<uint8_t>(host.read()));
    }

    while (radarSerial.available() > 0) {
      host.write(static_cast<uint8_t>(radarSerial.read()));
    }

    delay(1);
  }
}

}  // namespace tongdou
