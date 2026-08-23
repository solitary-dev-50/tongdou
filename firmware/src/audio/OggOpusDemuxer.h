#pragma once

#include <Arduino.h>

#include <functional>

namespace tongdou {

struct OggOpusPacket {
  const uint8_t* data = nullptr;
  size_t size = 0;
  uint32_t sampleRate = 48000;
  uint16_t frameDurationMs = 0;
};

class OggOpusDemuxer {
 public:
  using PacketHandler = std::function<bool(const OggOpusPacket& packet)>;

  void reset();
  bool process(const uint8_t* data, size_t size, const PacketHandler& onPacket);
  bool finish() const;
  const char* error() const;

 private:
  enum class ParseState : uint8_t {
    FindPage,
    ParseHeader,
    ParseSegments,
    ParseData,
  };

  struct OpusInfo {
    bool headSeen = false;
    bool tagsSeen = false;
    bool mono = false;
    uint32_t sampleRate = 48000;
  };

  struct Context {
    bool packetContinued = false;
    uint8_t header[27] = {};
    uint8_t segmentTable[255] = {};
    uint8_t packetBuffer[2048] = {};
    size_t packetLength = 0;
    size_t segmentCount = 0;
    size_t segmentIndex = 0;
    size_t dataOffset = 0;
    size_t bytesNeeded = 4;
    size_t segmentRemaining = 0;
    size_t bodySize = 0;
    size_t bodyOffset = 0;
  };

  static int packetDurationMs(const uint8_t* data, size_t size);
  void fail(const char* code);

  ParseState state_ = ParseState::FindPage;
  Context context_;
  OpusInfo opus_;
  bool failed_ = false;
  const char* error_ = "";
  size_t packetCount_ = 0;
};

}  // namespace tongdou
