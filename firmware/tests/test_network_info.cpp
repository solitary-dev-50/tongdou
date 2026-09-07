#include <assert.h>
#include <stdio.h>
#include <WiFi.h>

#include "network/NetworkInfoService.h"
#include "realtime/VoiceVisualFeedback.h"

using namespace tongdou;

uint32_t fakeMillis = 0;
TestSerial Serial;
TestWiFi WiFi;

int main() {
  FaceDisplay display;
  ReminderAlertService reminder;
  DeviceLocaleStore locale;
  NetworkInfoService network(display, reminder, locale);
  RealtimeVoiceService voice;
  LedPixel led;
  VoiceVisualFeedback visual(voice, display, led);
  visual.begin();
  auto update = [&]() {
    network.update(voice.value.state == RealtimeVoiceState::AwaitingResponse ||
                   voice.value.state == RealtimeVoiceState::Ending ||
                   voice.value.speechOutputActive,
                   voice.value);
    visual.update(reminder.blocksVoiceInteraction(), network.active());
  };

  // 没有地址时不能伪造 0.0.0.0 或上次地址。
  assert(network.show(voice.value).code == "network_unavailable");
  assert(display.revision() == 0);
  WiFi.connected = true;
  assert(network.show(voice.value).code == "network_unavailable");
  WiFi.station = IPAddress("192.168.10.123");
  auto result = network.show(voice.value);
  assert(result.displayed && result.url == "http://192.168.10.123/");
  assert(result.accessMode == "same_wifi");
  const auto firstBitmap = display.bitmap;
  const auto revision = display.revision();

  // 工具执行后的等待、说话和下一轮收音都不能抹掉地址；灯光仍更新。
  for (auto state : {RealtimeVoiceState::AwaitingResponse,
                     RealtimeVoiceState::Ending, RealtimeVoiceState::Listening}) {
    voice.value.state = state;
    update();
    assert(display.revision() == revision);
    assert(display.bitmap == firstBitmap);
  }
  assert(led.revision() == 3);
  // 进入下一轮监听本身不收起；服务端收到下一句转写后立即收起。
  voice.value.backendTextMessagesReceived = 1;
  voice.value.backendLastMessageType = "tts";
  voice.value.backendLastText = "我的地址已经显示在屏幕上了";
  update();
  assert(network.active());
  voice.value.backendTextMessagesReceived = 2;
  voice.value.backendLastMessageType = "stt";
  voice.value.backendLastText = "现在几点了";
  update();
  assert(!network.active());
  assert(network.show(voice.value).displayed);
  fakeMillis = 29999;
  update();
  assert(network.active());
  fakeMillis = 30000;
  update();
  assert(!network.active() && display.expression == FaceExpression::Listening);

  // 每次查询使用新地址，重复查询重新给足阅读时间。
  WiFi.station = IPAddress("10.0.0.8");
  result = network.show(voice.value);
  assert(result.ip == "10.0.0.8" && display.bitmap != firstBitmap);
  fakeMillis = 50000;
  assert(network.show(voice.value).displayed);
  fakeMillis = 79999;
  update();
  assert(network.active());
  fakeMillis = 80000;
  update();
  assert(!network.active());

  // 地址变化、断网不能留下过期入口。
  assert(network.show(voice.value).displayed);
  WiFi.station = IPAddress("10.0.0.9");
  update();
  assert(!network.active());
  assert(network.show(voice.value).displayed);
  WiFi.connected = false;
  update();
  assert(!network.active());
  assert(network.show(voice.value).ip.empty());

  // 配网热点是实际启用时才可用；同时联网时优先正常局域网地址。
  WiFi.mode = WIFI_AP;
  WiFi.portal = IPAddress("192.168.4.1");
  result = network.show(voice.value);
  assert(result.displayed && result.accessMode == "setup_hotspot");
  assert(result.url == "http://192.168.4.1/");
  WiFi.connected = true;
  result = network.show(voice.value);
  assert(result.accessMode == "same_wifi" && result.ip == "10.0.0.9");

  // 屏幕不可用不能返回显示成功，但仍返回查到的真实地址。
  display.available = false;
  result = network.show(voice.value);
  assert(!result.displayed && result.code == "display_unavailable");
  assert(result.ip == "10.0.0.9");
  display.available = true;

  // 到点提醒优先，不能因为地址页仍在计时就覆盖提醒。
  assert(network.show(voice.value).displayed);
  reminder.busy = true;
  display.show(FaceExpression::Serious);
  const auto reminderRevision = display.revision();
  update();
  assert(!network.active() && display.revision() == reminderRevision);
  assert(network.show(voice.value).code == "reminder_busy");
  reminder.busy = false;

  // 普通语音结束时，灯熄灭但地址保留；到期能正常收起。
  assert(network.show(voice.value).displayed);
  voice.value.state = RealtimeVoiceState::Listening;
  update();
  voice.value.state = RealtimeVoiceState::Idle;
  update();
  fakeMillis += 250;
  update();
  assert(network.active() && led.dark);
  fakeMillis += 30000;
  update();
  assert(!network.active() && display.expression == FaceExpression::Sleep);

  // 芯片毫秒计数回绕后，30 秒显示期限仍准确。
  fakeMillis = UINT32_MAX - 1000;
  assert(network.show(voice.value).displayed);
  fakeMillis += 29999;
  network.update(false, voice.value);
  assert(network.active());
  ++fakeMillis;
  network.update(false, voice.value);
  assert(!network.active());

  // 回复超过 30 秒仍保留地址，说完后给足 30 秒阅读时间。
  assert(network.show(voice.value).displayed);
  voice.value.state = RealtimeVoiceState::AwaitingResponse;
  fakeMillis += 40000;
  update();
  assert(network.active());
  voice.value.speechOutputActive = true;
  fakeMillis += 40000;
  update();
  assert(network.active());
  voice.value.speechOutputActive = false;
  voice.value.state = RealtimeVoiceState::Listening;
  fakeMillis += 29999;
  update();
  assert(network.active());
  ++fakeMillis;
  update();
  assert(!network.active());

  // 导出真实渲染结果，检查最长地址和中英文提示的小屏幕排版。
  WiFi.station = IPAddress("255.255.255.255");
  FILE* preview = fopen("network_info.bin", "wb");
  assert(preview);
  for (const auto* language : {"zh-CN", "en-GB"}) {
    locale.value = language;
    assert(network.show(voice.value).displayed);
    fwrite(display.bitmap.data(), 1, display.bitmap.size(), preview);
  }
  fclose(preview);
  puts("Network info: address, voice visuals, timeout, reconnect, portal, reminder and display checks passed.");
}
