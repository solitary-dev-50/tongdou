#include "hardware/VoiceUart.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr uint32_t kCiProtocolBaudrate = 9600;

}  // namespace

void VoiceUart::begin() {
  Serial1.begin(
      kCiProtocolBaudrate,
      SERIAL_8N1,
      static_cast<int>(pins::VOICE_UART_RX),
      static_cast<int>(pins::VOICE_UART_TX));
}

bool VoiceUart::readEvent(ScenarioEvent& event) {
  while (Serial1.available() > 0) {
    const uint8_t byte = static_cast<uint8_t>(Serial1.read());
    VoiceCommandFrame frame;
    if (!parser_.feed(byte, frame)) {
      continue;
    }

    if (mapper_.map(frame, event)) {
      return true;
    }
  }

  return false;
}

void VoiceUart::playVoice(uint16_t voiceId) {
  if (voiceId == 0) {
    return;
  }

  const VoicePlaybackFrame frame = protocol_.makePlayVoiceFrame(voiceId, nextSequence());
  Serial1.write(frame.bytes, frame.length);
}

uint8_t VoiceUart::nextSequence() {
  ++sequence_;
  if (sequence_ == 0) {
    sequence_ = 1;
  }
  return sequence_;
}

}  // namespace tongdou
