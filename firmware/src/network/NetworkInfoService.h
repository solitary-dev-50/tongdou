#pragma once

#include <Arduino.h>

#include "hardware/FaceDisplay.h"
#include "reminder/ReminderAlertService.h"
#include "realtime/RealtimeVoiceService.h"
#include "system/DeviceLocaleStore.h"

namespace tongdou {

struct NetworkInfoResult {
  bool displayed = false;
  String code;
  String ip;
  String url;
  String accessMode;
};

// 只负责把本机当前网络入口展示给用户。
class NetworkInfoService {
 public:
  NetworkInfoService(FaceDisplay& display, ReminderAlertService& reminderAlert,
                     DeviceLocaleStore& locale);
  NetworkInfoResult show(const RealtimeVoiceSnapshot& voiceSnapshot);
  void update(bool voiceReplyActive, const RealtimeVoiceSnapshot& voiceSnapshot);
  bool active() const;

 private:
  bool draw();
  void dismiss();

  FaceDisplay& display_;
  ReminderAlertService& reminderAlert_;
  DeviceLocaleStore& locale_;
  String ip_;
  bool portal_ = false;
  bool active_ = false;
  uint32_t shownAtMs_ = 0;
  uint32_t displayRevision_ = 0;
  uint32_t voiceTextMessageBaseline_ = 0;
  bool voiceTextBaselineReady_ = false;
};

}  // namespace tongdou
