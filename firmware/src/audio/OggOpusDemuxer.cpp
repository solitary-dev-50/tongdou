#include "audio/OggOpusDemuxer.h"

#include <algorithm>
#include <cstring>

namespace tongdou {

void OggOpusDemuxer::reset() {
  state_ = ParseState::FindPage;
  context_ = {};
  opus_ = {};
  failed_ = false;
  error_ = "";
  packetCount_ = 0;
}

bool OggOpusDemuxer::process(const uint8_t* data, size_t size,
                             const PacketHandler& onPacket) {
  if (data == nullptr || size == 0) {
    fail("invalid_ogg_buffer");
    return false;
  }

  size_t processed = 0;
  while (processed < size && !failed_) {
    switch (state_) {
      case ParseState::FindPage: {
        if (context_.bytesNeeded < 4) {
          const size_t toCopy = std::min(size - processed, context_.bytesNeeded);
          memcpy(context_.header + (4 - context_.bytesNeeded), data + processed, toCopy);
          processed += toCopy;
          context_.bytesNeeded -= toCopy;
          if (context_.bytesNeeded > 0) {
            return true;
          }
          if (memcmp(context_.header, "OggS", 4) == 0) {
            state_ = ParseState::ParseHeader;
            context_.dataOffset = 4;
            context_.bytesNeeded = 23;
          } else {
            memmove(context_.header, context_.header + 1, 3);
            context_.bytesNeeded = 1;
          }
          break;
        }

        const size_t remaining = size - processed;
        bool found = false;
        size_t offset = 0;
        for (; offset + 4 <= remaining; ++offset) {
          if (memcmp(data + processed + offset, "OggS", 4) == 0) {
            found = true;
            break;
          }
        }
        if (!found) {
          const size_t partialLength = remaining - offset;
          if (partialLength > 0) {
            memcpy(context_.header, data + processed + offset, partialLength);
            context_.bytesNeeded = 4 - partialLength;
            processed += offset + partialLength;
          } else {
            processed += offset;
          }
          return true;
        }

        processed += offset + 4;
        state_ = ParseState::ParseHeader;
        context_.dataOffset = 4;
        context_.bytesNeeded = 23;
        break;
      }

      case ParseState::ParseHeader: {
        const size_t toCopy = std::min(size - processed, context_.bytesNeeded);
        memcpy(context_.header + context_.dataOffset, data + processed, toCopy);
        processed += toCopy;
        context_.dataOffset += toCopy;
        context_.bytesNeeded -= toCopy;
        if (context_.bytesNeeded > 0) {
          return true;
        }

        if (context_.header[4] != 0) {
          fail("invalid_ogg_version");
          return false;
        }
        context_.segmentCount = context_.header[26];
        if (context_.segmentCount == 0) {
          state_ = ParseState::FindPage;
          context_.bytesNeeded = 4;
          context_.dataOffset = 0;
          break;
        }
        state_ = ParseState::ParseSegments;
        context_.bytesNeeded = context_.segmentCount;
        context_.dataOffset = 0;
        break;
      }

      case ParseState::ParseSegments: {
        const size_t toCopy = std::min(size - processed, context_.bytesNeeded);
        memcpy(context_.segmentTable + context_.dataOffset, data + processed, toCopy);
        processed += toCopy;
        context_.dataOffset += toCopy;
        context_.bytesNeeded -= toCopy;
        if (context_.bytesNeeded > 0) {
          return true;
        }

        context_.bodySize = 0;
        for (size_t i = 0; i < context_.segmentCount; ++i) {
          context_.bodySize += context_.segmentTable[i];
        }
        context_.bodyOffset = 0;
        context_.segmentIndex = 0;
        context_.segmentRemaining = 0;
        state_ = ParseState::ParseData;
        break;
      }

      case ParseState::ParseData: {
        while (context_.segmentIndex < context_.segmentCount && processed < size) {
          uint8_t segmentLength = context_.segmentTable[context_.segmentIndex];
          if (context_.segmentRemaining > 0) {
            segmentLength = context_.segmentRemaining;
          } else {
            context_.segmentRemaining = segmentLength;
          }

          if (context_.packetLength + segmentLength > sizeof(context_.packetBuffer)) {
            fail("ogg_packet_too_large");
            return false;
          }

          const size_t toCopy = std::min(size - processed,
                                         static_cast<size_t>(segmentLength));
          memcpy(context_.packetBuffer + context_.packetLength, data + processed, toCopy);
          processed += toCopy;
          context_.packetLength += toCopy;
          context_.bodyOffset += toCopy;
          context_.segmentRemaining -= toCopy;
          if (context_.segmentRemaining > 0) {
            return true;
          }

          const bool segmentContinues = context_.segmentTable[context_.segmentIndex] == 255;
          if (!segmentContinues && context_.packetLength > 0) {
            if (!opus_.headSeen) {
              if (context_.packetLength < 19 ||
                  memcmp(context_.packetBuffer, "OpusHead", 8) != 0) {
                fail("missing_opus_head");
                return false;
              }
              opus_.headSeen = true;
              opus_.mono = context_.packetBuffer[9] == 1;
              if (!opus_.mono) {
                fail("ogg_opus_not_mono");
                return false;
              }
              const uint32_t inputSampleRate =
                  static_cast<uint32_t>(context_.packetBuffer[12]) |
                  (static_cast<uint32_t>(context_.packetBuffer[13]) << 8) |
                  (static_cast<uint32_t>(context_.packetBuffer[14]) << 16) |
                  (static_cast<uint32_t>(context_.packetBuffer[15]) << 24);
              switch (inputSampleRate) {
                case 8000:
                case 12000:
                case 16000:
                case 24000:
                case 48000:
                  opus_.sampleRate = inputSampleRate;
                  break;
                default:
                  opus_.sampleRate = 48000;
                  break;
              }
            } else if (!opus_.tagsSeen) {
              if (memcmp(context_.packetBuffer, "OpusTags", 8) != 0) {
                fail("missing_opus_tags");
                return false;
              }
              opus_.tagsSeen = true;
            } else {
              const int durationMs =
                  packetDurationMs(context_.packetBuffer, context_.packetLength);
              if (durationMs <= 0) {
                fail("unsupported_opus_packet_duration");
                return false;
              }
              ++packetCount_;
              if (onPacket) {
                OggOpusPacket packet;
                packet.data = context_.packetBuffer;
                packet.size = context_.packetLength;
                packet.sampleRate = opus_.sampleRate;
                packet.frameDurationMs = static_cast<uint16_t>(durationMs);
                if (!onPacket(packet)) {
                  fail("ogg_packet_handler_failed");
                  return false;
                }
              }
            }
            context_.packetLength = 0;
            context_.packetContinued = false;
          } else if (segmentContinues) {
            context_.packetContinued = true;
          } else {
            context_.packetLength = 0;
            context_.packetContinued = false;
          }

          ++context_.segmentIndex;
          context_.segmentRemaining = 0;
        }

        if (context_.segmentIndex == context_.segmentCount) {
          if (!context_.packetContinued) {
            context_.packetLength = 0;
          }
          state_ = ParseState::FindPage;
          context_.bytesNeeded = 4;
          context_.dataOffset = 0;
        }
        break;
      }
    }
  }

  return !failed_;
}

bool OggOpusDemuxer::finish() const {
  return !failed_ && opus_.headSeen && opus_.tagsSeen && opus_.mono && packetCount_ > 0 &&
         state_ == ParseState::FindPage && context_.bytesNeeded == 4 &&
         context_.packetLength == 0;
}

const char* OggOpusDemuxer::error() const {
  return error_;
}

int OggOpusDemuxer::packetDurationMs(const uint8_t* data, size_t size) {
  if (data == nullptr || size == 0) {
    return -1;
  }

  const uint8_t toc = data[0];
  const uint8_t config = toc >> 3;
  int frameDurationUs = 0;
  if (config < 12) {
    static constexpr int kSilkDurationsUs[] = {10000, 20000, 40000, 60000};
    frameDurationUs = kSilkDurationsUs[config & 0x03];
  } else if (config < 16) {
    frameDurationUs = (config & 0x01) ? 20000 : 10000;
  } else {
    static constexpr int kCeltDurationsUs[] = {2500, 5000, 10000, 20000};
    frameDurationUs = kCeltDurationsUs[config & 0x03];
  }

  int frameCount = 0;
  switch (toc & 0x03) {
    case 0:
      frameCount = 1;
      break;
    case 1:
    case 2:
      frameCount = 2;
      break;
    case 3:
      if (size < 2) {
        return -1;
      }
      frameCount = data[1] & 0x3f;
      break;
  }

  const int packetDurationUs = frameDurationUs * frameCount;
  if (frameCount == 0 || packetDurationUs > 120000 || packetDurationUs % 1000 != 0) {
    return -1;
  }
  return packetDurationUs / 1000;
}

void OggOpusDemuxer::fail(const char* code) {
  failed_ = true;
  error_ = code;
}

}  // namespace tongdou
