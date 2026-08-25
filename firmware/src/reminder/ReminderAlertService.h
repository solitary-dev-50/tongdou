#pragma once

#include <Arduino.h>

#include "audio/AudioCuePlayer.h"
#include "hardware/FaceDisplay.h"
#include "realtime/RealtimeVoiceService.h"
#include "reminder/ReminderStore.h"

namespace tongdou {

struct ReminderAlertCompletion {
  uint32_t reminderId = 0;
  bool online = false;
};

class ReminderAlertService {
 public:
  ReminderAlertService(FaceDisplay& faceDisplay, AudioCuePlayer& audioCuePlayer,
                       RealtimeVoiceService& voiceService);

  void begin();
  void update();
  bool start(const ReminderRecord& reminder, const String& locale);
  bool canStart() const;
  bool blocksVoiceInteraction() const;
  bool consumeCompletedEvent(ReminderAlertCompletion& completion);
  void dismissDisplayedReminder();

 private:
  enum class Phase : uint8_t {
    Idle,
    WaitingForBackend,
    OnlineAnnouncement,
    Displaying,
  };

  void showReminderBitmap();
  void playOfflineFallback(const String& reason);
  void complete(bool online);

  static constexpr unsigned long kBackendWaitMs = 2500;

  FaceDisplay& faceDisplay_;
  AudioCuePlayer& audioCuePlayer_;
  RealtimeVoiceService& voiceService_;
  ReminderRecord reminder_;
  String locale_;
  Phase phase_ = Phase::Idle;
  bool displayActive_ = false;
  bool completionPending_ = false;
  bool completionOnline_ = false;
  uint32_t displayRevision_ = 0;
  unsigned long backendDeadlineMs_ = 0;
};

}  // namespace tongdou
