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

}  // namespace tongdou
