"""在电脑上编译真实网络信息和语音视觉代码，只替换硬件依赖。"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


STUBS = {
    "Arduino.h": r'''
#pragma once
#include <stdint.h>
#include <string>
using String = std::string;
extern uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
#define F(text) text
struct TestSerial {
  template<class T> void print(T) {}
  template<class T> void println(T) {}
};
extern TestSerial Serial;
''',
    "WiFi.h": r'''
#pragma once
#include <Arduino.h>
constexpr int WL_CONNECTED = 3;
constexpr int WIFI_AP = 2;
class IPAddress {
 public:
  IPAddress(const char* value = "0.0.0.0") : text(value) {}
  explicit operator uint32_t() const { return text == "0.0.0.0" ? 0 : 1; }
  String toString() const { return text; }
 private:
  String text;
};
struct TestWiFi {
  bool connected = false;
  int mode = 0;
  IPAddress station, portal;
  int status() const { return connected ? WL_CONNECTED : 0; }
  int getMode() const { return mode; }
  IPAddress localIP() const { return station; }
  IPAddress softAPIP() const { return portal; }
};
extern TestWiFi WiFi;
''',
    "hardware/FaceDisplay.h": r'''
#pragma once
#include <Arduino.h>
#include <array>
#include <algorithm>
namespace tongdou {
enum class FaceExpression { Sleep, Listening, Serious, Smile };
class FaceDisplay {
 public:
  bool available = true;
  std::array<uint8_t, 768> bitmap = {};
  FaceExpression expression = FaceExpression::Sleep;
  bool ready() const { return available; }
  uint32_t revision() const { return revision_; }
  void show(FaceExpression value) { expression = value; ++revision_; }
  bool showBitmap128x48(const uint8_t* data, size_t size) {
    if (!available || size != bitmap.size()) return false;
    std::copy(data, data + size, bitmap.begin());
    ++revision_;
    return true;
  }
 private:
  uint32_t revision_ = 0;
};
}
''',
    "hardware/LedPixel.h": r'''
#pragma once
#include <stdint.h>
namespace tongdou {
struct RgbColor { uint8_t r, g, b; };
class LedPixel {
 public:
  bool dark = true;
  void show(RgbColor) { dark = false; ++revision_; }
  void off() { dark = true; ++revision_; }
  uint32_t revision() const { return revision_; }
 private:
  uint32_t revision_ = 0;
};
}
''',
    "reminder/ReminderAlertService.h": r'''
#pragma once
namespace tongdou {
struct ReminderAlertService {
  bool busy = false;
  bool blocksVoiceInteraction() const { return busy; }
  void dismissDisplayedReminder() {}
};
}
''',
    "system/DeviceLocaleStore.h": r'''
#pragma once
#include <Arduino.h>
namespace tongdou {
struct DeviceLocaleStore {
  String value = "zh-CN";
  String locale() const { return value; }
};
}
''',
    "realtime/RealtimeVoiceService.h": r'''
#pragma once
#include <Arduino.h>
namespace tongdou {
enum class RealtimeVoiceState { Idle, Listening, AwaitingResponse, Ending, Failed };
struct RealtimeVoiceSnapshot {
  RealtimeVoiceState state = RealtimeVoiceState::Idle;
  uint32_t backendTextMessagesReceived = 0;
  String backendLastMessageType;
  String backendLastText;
  bool reminderAnnouncementActive = false;
  bool speechOutputActive = false;
};
struct RealtimeVoiceService {
  RealtimeVoiceSnapshot value;
  RealtimeVoiceSnapshot snapshot() const { return value; }
};
}
''',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path)
    parser.add_argument("--preview", type=Path, help="可选：导出真实显示位图预览，需要 Pillow")
    args = parser.parse_args()
    firmware = Path(__file__).resolve().parents[1]
    compiler = args.compiler or shutil.which("g++")
    if not compiler:
        compiler = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe"
    if not Path(compiler).is_file():
        parser.error("找不到电脑端 C++ 编译器，请通过 --compiler 指定。")
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
    with tempfile.TemporaryDirectory(prefix="tongdou-network-test-") as directory:
        work = Path(directory)
        for name, source in STUBS.items():
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(source, encoding="utf-8")
        binary = work / ("test.exe" if os.name == "nt" else "test")
        subprocess.run([
            str(compiler), "-std=c++17", "-Wall", "-Wextra", "-Werror",
            "-I", str(work), "-I", str(firmware / "src"),
            str(firmware / "tests/test_network_info.cpp"),
            str(firmware / "src/network/NetworkInfoService.cpp"),
            str(firmware / "src/realtime/VoiceVisualFeedback.cpp"),
            "-o", str(binary),
        ], check=True, env=env)
        subprocess.run([str(binary)], check=True, cwd=work, env=env)
        if args.preview:
            from PIL import Image

            data = (work / "network_info.bin").read_bytes()
            preview = Image.new("1", (128, 128))
            for panel in range(2):
                for y in range(48):
                    for x in range(128):
                        value = data[panel * 768 + (y // 8) * 128 + x]
                        preview.putpixel((x, panel * 64 + y + 8), bool(value & (1 << (y % 8))))
            args.preview.parent.mkdir(parents=True, exist_ok=True)
            preview.resize((384, 384), Image.Resampling.NEAREST).save(args.preview)


if __name__ == "__main__":
    main()
