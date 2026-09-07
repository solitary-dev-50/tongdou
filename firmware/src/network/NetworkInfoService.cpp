#include "network/NetworkInfoService.h"

#include <WiFi.h>
#include <string.h>

#include "network/NetworkInfoLabels.h"

namespace tongdou {
namespace {

constexpr uint32_t kDisplayMs = 30000;

IPAddress currentAddress(bool& portal) {
  portal = false;
  if (WiFi.status() == WL_CONNECTED) {
    const IPAddress address = WiFi.localIP();
    if (static_cast<uint32_t>(address) != 0) {
      return address;
    }
  }
  if ((WiFi.getMode() & WIFI_AP) != 0) {
    portal = true;
    return WiFi.softAPIP();
  }
  return IPAddress();
}

// 地址最多 15 个字符，使用 7 像素间距，完整落在 128 像素宽度内。
void drawAddress(uint8_t* bitmap, const String& ip) {
  constexpr uint8_t digits[10][5] = {
      {0x3e, 0x51, 0x49, 0x45, 0x3e}, {0x00, 0x42, 0x7f, 0x40, 0x00},
      {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4b, 0x31},
      {0x18, 0x14, 0x12, 0x7f, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
      {0x3c, 0x4a, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
      {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1e},
  };
  const size_t start = (128 - (ip.length() * 7 - 2)) / 2;
  for (size_t i = 0; i < ip.length(); ++i) {
    const char ch = ip[i];
    const size_t offset = 128 * 2 + start + i * 7;
    if (ch >= '0' && ch <= '9') {
      memcpy(bitmap + offset, digits[ch - '0'], 5);
    } else if (ch == '.') {
      bitmap[offset + 2] = 0x60;
      bitmap[offset + 3] = 0x60;
    }
  }
}

}  // namespace

NetworkInfoService::NetworkInfoService(FaceDisplay& display,
                                       ReminderAlertService& reminderAlert,
                                       DeviceLocaleStore& locale)
    : display_(display), reminderAlert_(reminderAlert), locale_(locale) {}

NetworkInfoResult NetworkInfoService::show(const RealtimeVoiceSnapshot& voiceSnapshot) {
  NetworkInfoResult result;
  bool portal = false;
  const IPAddress address = currentAddress(portal);
  if (static_cast<uint32_t>(address) == 0) {
    dismiss();
    result.code = "network_unavailable";
    return result;
  }
  result.ip = address.toString();
  result.url = "http://" + result.ip + "/";
  result.accessMode = portal ? "setup_hotspot" : "same_wifi";
  if (reminderAlert_.blocksVoiceInteraction()) {
    result.code = "reminder_busy";
    return result;
  }
  if (!display_.ready()) {
    result.code = "display_unavailable";
    return result;
  }

  ip_ = result.ip;
  portal_ = portal;
  if (!draw()) {
    result.code = "display_unavailable";
    return result;
  }
  reminderAlert_.dismissDisplayedReminder();
  active_ = true;
  shownAtMs_ = millis();
  voiceTextMessageBaseline_ = voiceSnapshot.backendTextMessagesReceived;
  voiceTextBaselineReady_ = true;
  result.displayed = true;
  result.code = "ok";
  return result;
}

void NetworkInfoService::update(bool voiceReplyActive,
                                const RealtimeVoiceSnapshot& voiceSnapshot) {
  if (!active_) {
    return;
  }
  // 到点提醒或其他明确的屏幕内容接管后，地址页不再抢回屏幕。
  if (reminderAlert_.blocksVoiceInteraction() ||
      display_.revision() != displayRevision_) {
    active_ = false;
    return;
  }
  if (voiceTextBaselineReady_ &&
      voiceSnapshot.backendTextMessagesReceived > voiceTextMessageBaseline_ &&
      voiceSnapshot.backendLastMessageType == "stt" &&
      voiceSnapshot.backendLastText.length() > 0) {
    // 自动进入下一轮监听不关闭；只有服务端确实识别出用户新一句话才关闭。
    dismiss();
    return;
  }
  // 等待回复或正在播报时不收起；说完后再给用户完整的阅读时间。
  if (voiceReplyActive) {
    shownAtMs_ = millis();
  }
  bool portal = false;
  const IPAddress address = currentAddress(portal);
  if (static_cast<uint32_t>(address) == 0 || portal != portal_ ||
      address.toString() != ip_ ||
      static_cast<uint32_t>(millis() - shownAtMs_) >= kDisplayMs) {
    dismiss();
  }
}

bool NetworkInfoService::active() const {
  return active_;
}

bool NetworkInfoService::draw() {
  uint8_t bitmap[768] = {};
  const bool chinese = locale_.locale() == "zh-CN";
  const uint8_t* labels = chinese
                              ? (portal_ ? kPortalLabelsZh : kNetworkLabelsZh)
                              : (portal_ ? kPortalLabelsEn : kNetworkLabelsEn);
  memcpy(bitmap, labels, 256);
  memcpy(bitmap + 512, labels + 256, 256);
  drawAddress(bitmap, ip_);
  if (!display_.showBitmap128x48(bitmap, sizeof(bitmap))) {
    return false;
  }
  displayRevision_ = display_.revision();
  return true;
}

void NetworkInfoService::dismiss() {
  if (active_ && display_.revision() == displayRevision_ &&
      !reminderAlert_.blocksVoiceInteraction()) {
    display_.show(FaceExpression::Sleep);
  }
  active_ = false;
  voiceTextBaselineReady_ = false;
}

}  // namespace tongdou
