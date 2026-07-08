#include "hardware/RadarSensor.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

HardwareSerial radarSerial(1);

constexpr uint32_t kRadarBaud = 115200;
constexpr uint16_t kCommandTimeoutMs = 300;
constexpr uint16_t kTargetFrameTimeoutMs = 500;
constexpr size_t kMaxFrameSize = 80;

constexpr uint16_t kEnableConfigAck = 0x01FF;
constexpr uint16_t kEndConfigAck = 0x01FE;
constexpr uint16_t kReadResolutionAck = 0x0111;
constexpr uint16_t kSetResolutionAck = 0x0101;
constexpr uint16_t kReadBasicConfigAck = 0x0112;
constexpr uint16_t kSetBasicConfigAck = 0x0102;
constexpr uint16_t kBackgroundCalibrationAck = 0x010B;
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
constexpr uint8_t kNearDeskConfigCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x07, 0x00, 0x02, 0x00, 0x01, 0x04, 0x05,
    0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
constexpr uint8_t kBackgroundCalibrationCommand[] = {
    0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x0B, 0x00, 0x04, 0x03, 0x02, 0x01};
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

bool readRadarReportFrame(uint8_t* payload, size_t payloadCapacity, size_t& payloadLength,
                          uint16_t timeoutMs) {
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
             !(buffer[0] == 0xF4 && buffer[1] == 0xF3 && buffer[2] == 0xF2 &&
               buffer[3] == 0xF1)) {
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

      const bool footerOk = buffer[totalLength - 4] == 0xF8 &&
                            buffer[totalLength - 3] == 0xF7 &&
                            buffer[totalLength - 2] == 0xF6 &&
                            buffer[totalLength - 1] == 0xF5;
      if (footerOk) {
        payloadLength = frameLength;
        const size_t copyLength = min(payloadLength, payloadCapacity);
        memcpy(payload, buffer + 6, copyLength);
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

}  // namespace

void RadarSensor::begin() {
  pinMode(pins::RADAR_OUT, INPUT);
  pinMode(pins::RADAR_RX_FROM_MODULE_TX, INPUT);
  radarSerial.begin(kRadarBaud, SERIAL_8N1, static_cast<int>(pins::RADAR_RX_FROM_MODULE_TX),
                    static_cast<int>(pins::RADAR_TX_TO_MODULE_RX));
  clearRadarInput();
}

RadarSnapshot RadarSensor::read() const {
  RadarSnapshot snapshot;
  snapshot.occupied = digitalRead(pins::RADAR_OUT) == HIGH;
  snapshot.rxLevelHigh = digitalRead(pins::RADAR_RX_FROM_MODULE_TX) == HIGH;
  return snapshot;
}

RadarBasicConfig RadarSensor::readBasicConfig() {
  RadarBasicConfig config;
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
  clearRadarInput();

  if (!sendAndWaitAck(kEnableConfigCommand, sizeof(kEnableConfigCommand), kEnableConfigAck)) {
    return false;
  }

  const bool configured =
      sendAndWaitAck(kNearDeskConfigCommand, sizeof(kNearDeskConfigCommand), kSetBasicConfigAck);
  sendAndWaitAck(kEndConfigCommand, sizeof(kEndConfigCommand), kEndConfigAck);
  return configured;
}

RadarCommandResult RadarSensor::applyResolution20cm() {
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

RadarTargetSnapshot RadarSensor::readTarget() {
  RadarTargetSnapshot snapshot;
  uint8_t payload[32] = {};
  size_t payloadLength = 0;

  clearRadarInput();
  snapshot.received =
      readRadarReportFrame(payload, sizeof(payload), payloadLength, kTargetFrameTimeoutMs);
  if (!snapshot.received || payloadLength < 11) {
    return snapshot;
  }

  const size_t rawLength = min(payloadLength, sizeof(snapshot.rawPayload));
  snapshot.rawPayloadLength = static_cast<uint8_t>(rawLength);
  memcpy(snapshot.rawPayload, payload, rawLength);

  if (payload[1] != 0xAA || payload[9] != 0x55) {
    snapshot.received = false;
    return snapshot;
  }

  // Normal report: type + 0xAA + state + moving distance/energy + static distance/energy.
  snapshot.targetState = payload[2];
  snapshot.movingDistanceCm = static_cast<uint16_t>(payload[3]) |
                              (static_cast<uint16_t>(payload[4]) << 8);
  snapshot.movingEnergy = payload[5];
  snapshot.staticDistanceCm = static_cast<uint16_t>(payload[6]) |
                              (static_cast<uint16_t>(payload[7]) << 8);
  snapshot.staticEnergy = payload[8];
  snapshot.hasTarget = snapshot.targetState >= 1 && snapshot.targetState <= 3;

  if (snapshot.targetState == 1) {
    snapshot.targetDistanceCm = snapshot.movingDistanceCm;
  } else if (snapshot.targetState == 2) {
    snapshot.targetDistanceCm = snapshot.staticDistanceCm;
  } else if (snapshot.targetState == 3) {
    if (snapshot.movingDistanceCm == 0) {
      snapshot.targetDistanceCm = snapshot.staticDistanceCm;
    } else if (snapshot.staticDistanceCm == 0) {
      snapshot.targetDistanceCm = snapshot.movingDistanceCm;
    } else {
      snapshot.targetDistanceCm = min(snapshot.movingDistanceCm, snapshot.staticDistanceCm);
    }
  }

  return snapshot;
}

}  // namespace tongdou
