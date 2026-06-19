#include "hardware/VoiceUart.h"

#include <Arduino.h>

#include "tongdou/Pins.h"

namespace tongdou {

void VoiceUart::begin() {
  Serial1.begin(
      115200,
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

    event = mapper_.map(frame);
    return true;
  }

  return false;
}

}  // namespace tongdou
