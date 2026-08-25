#include "reminder/ReminderAlertService.h"

namespace tongdou {

ReminderAlertService::ReminderAlertService(FaceDisplay& faceDisplay,
                                           AudioCuePlayer& audioCuePlayer,
                                           RealtimeVoiceService& voiceService)
    : faceDisplay_(faceDisplay),
      audioCuePlayer_(audioCuePlayer),
      voiceService_(voiceService) {}

void ReminderAlertService::begin() {
  phase_ = Phase::Idle;
  displayActive_ = false;
}

void ReminderAlertService::update() {
  if (displayActive_ && faceDisplay_.revision() != displayRevision_) {
    showReminderBitmap();
  }

  if (phase_ == Phase::OnlineAnnouncement) {
    if (voiceService_.consumeReminderCompletedEvent()) {
      phase_ = Phase::Displaying;
      Serial.print(F("reminder alert online complete id="));
      Serial.println(reminder_.id);
      return;
    }
    if (voiceService_.consumeReminderFailureEvent()) {
      const RealtimeVoiceSnapshot snapshot = voiceService_.snapshot();
      playOfflineFallback(snapshot.lastErrorCode);
    }
    return;
  }

  if (phase_ != Phase::WaitingForBackend) {
    return;
  }

  const RealtimeVoiceSnapshot snapshot = voiceService_.snapshot();
  if (!snapshot.backendConnected) {
    playOfflineFallback("backend_offline");
    return;
  }
  if (snapshot.backendReady && snapshot.state == RealtimeVoiceState::Idle) {
    if (voiceService_.requestReminderAnnouncement(reminder_.id, reminder_.text,
                                                 locale_)) {
      phase_ = Phase::OnlineAnnouncement;
      return;
    }
    playOfflineFallback("request_failed");
    return;
  }
  if (static_cast<long>(millis() - backendDeadlineMs_) >= 0) {
    playOfflineFallback("backend_busy_timeout");
  }
}

bool ReminderAlertService::start(const ReminderRecord& reminder,
                                 const String& locale) {
  if (!canStart() || reminder.id == 0 || reminder.text[0] == '\0') {
    return false;
  }

  reminder_ = reminder;
  locale_ = locale;
  displayActive_ = reminder_.hasDisplayBitmap();
  backendDeadlineMs_ = millis() + kBackendWaitMs;
  phase_ = Phase::WaitingForBackend;
  if (displayActive_) {
    showReminderBitmap();
  } else {
    Serial.print(F("reminder display bitmap missing id="));
    Serial.println(reminder_.id);
  }
  Serial.print(F("reminder alert start id="));
  Serial.print(reminder_.id);
  Serial.print(F(" locale="));
  Serial.print(locale_);
  Serial.print(F(" display="));
  Serial.println(displayActive_ ? 1 : 0);
  return true;
}

bool ReminderAlertService::canStart() const {
  return phase_ == Phase::Idle || phase_ == Phase::Displaying;
}

bool ReminderAlertService::blocksVoiceInteraction() const {
  return phase_ == Phase::WaitingForBackend ||
         phase_ == Phase::OnlineAnnouncement;
}

void ReminderAlertService::dismissDisplayedReminder() {
  if (phase_ != Phase::Displaying) {
    return;
  }
  displayActive_ = false;
  phase_ = Phase::Idle;
  Serial.print(F("reminder display dismissed id="));
  Serial.println(reminder_.id);
}

void ReminderAlertService::showReminderBitmap() {
  if (!displayActive_ || !reminder_.hasDisplayBitmap()) {
    return;
  }
  if (faceDisplay_.showBitmap128x48(reminder_.displayBitmap,
                                   reminder_.displayBitmapLength)) {
    displayRevision_ = faceDisplay_.revision();
  }
}

void ReminderAlertService::playOfflineFallback(const String& reason) {
  const OfflineNoticePlaybackResult result =
      audioCuePlayer_.playOfflineNotice(locale_);
  phase_ = Phase::Displaying;
  Serial.print(F("reminder alert fallback id="));
  Serial.print(reminder_.id);
  Serial.print(F(" reason="));
  Serial.print(reason);
  Serial.print(F(" speech_ok="));
  Serial.print(result.speech.ok ? 1 : 0);
  Serial.print(F(" speech_drained="));
  Serial.print(result.speechDrained ? 1 : 0);
  Serial.print(F(" tones="));
  Serial.println(result.tonesPlayed);
}

}  // namespace tongdou
