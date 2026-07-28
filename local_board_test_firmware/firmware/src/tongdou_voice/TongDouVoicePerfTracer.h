#pragma once

#include <Arduino.h>

namespace tongdou {

class TongDouVoicePerfTracer {
 public:
  void reset();
  void listenStart();
  void firstOpusEncoded();
  void firstAudioSent();
  void listenStop();
  void firstTtsPacketReceived();
  void speakerStart();
  void ttsDone();

 private:
  void printMark(const char* name, unsigned long nowMs) const;

  unsigned long listenStartMs_ = 0;
  bool firstOpusEncodedLogged_ = false;
  bool firstAudioSentLogged_ = false;
  bool firstTtsPacketLogged_ = false;
  bool speakerStartLogged_ = false;
};

}  // namespace tongdou
