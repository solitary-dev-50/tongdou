#include "tongdou_voice/TongDouVoicePerfTracer.h"

namespace tongdou {

void TongDouVoicePerfTracer::reset() {
  listenStartMs_ = 0;
  firstOpusEncodedLogged_ = false;
  firstAudioSentLogged_ = false;
  firstTtsPacketLogged_ = false;
  speakerStartLogged_ = false;
}

void TongDouVoicePerfTracer::listenStart() {
  reset();
  listenStartMs_ = millis();
  printMark("listen_start", listenStartMs_);
}

void TongDouVoicePerfTracer::firstOpusEncoded() {
  if (firstOpusEncodedLogged_) {
    return;
  }

  firstOpusEncodedLogged_ = true;
  printMark("first_opus_encoded", millis());
}

void TongDouVoicePerfTracer::firstAudioSent() {
  if (firstAudioSentLogged_) {
    return;
  }

  firstAudioSentLogged_ = true;
  printMark("first_audio_sent", millis());
}

void TongDouVoicePerfTracer::listenStop() {
  printMark("listen_stop", millis());
}

void TongDouVoicePerfTracer::firstTtsPacketReceived() {
  if (firstTtsPacketLogged_) {
    return;
  }

  firstTtsPacketLogged_ = true;
  printMark("first_tts_packet_received", millis());
}

void TongDouVoicePerfTracer::speakerStart() {
  if (speakerStartLogged_) {
    return;
  }

  speakerStartLogged_ = true;
  printMark("speaker_start", millis());
}

void TongDouVoicePerfTracer::ttsDone() {
  printMark("tts_done", millis());
}

void TongDouVoicePerfTracer::printMark(const char* name, unsigned long nowMs) const {
  Serial.print("[voice] ");
  Serial.print(name);
  Serial.print(" t=");
  Serial.print(nowMs);
  if (listenStartMs_ > 0 && nowMs >= listenStartMs_) {
    Serial.print(" dt=");
    Serial.print(nowMs - listenStartMs_);
  }
  Serial.println();
}

}  // namespace tongdou
