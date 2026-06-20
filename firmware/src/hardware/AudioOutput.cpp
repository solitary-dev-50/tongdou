#include "hardware/AudioOutput.h"

#include "tongdou/Pins.h"

namespace tongdou {

void AudioOutput::begin() {
  Serial.print("audio output pins bclk=");
  Serial.print(static_cast<int>(pins::I2S_SPK_BCLK));
  Serial.print(" lrclk=");
  Serial.print(static_cast<int>(pins::I2S_SPK_LRCLK));
  Serial.print(" data=");
  Serial.println(static_cast<int>(pins::I2S_SPK_DATA));
}

void AudioOutput::playClip(uint16_t clipId, const char* text) {
  if (clipId == 0) {
    return;
  }

  Serial.print("audio clip=");
  Serial.print(clipId);
  Serial.print(" text=");
  Serial.println(text == nullptr ? "" : text);
}

}  // namespace tongdou
