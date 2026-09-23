"""在电脑上运行真实剧情执行器与陀螺仪动作代码，验证短动作和停止行为。"""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile


STUBS = {
    "Arduino.h": r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <algorithm>
using __FlashStringHelper = char;
#define F(x) x
#define constrain(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
extern unsigned long fakeMillis;
inline unsigned long millis() { return fakeMillis; }
struct TestSerial {
  template<class T> void print(T) {}
  template<class T> void println(T) {}
};
extern TestSerial Serial;
''',
    "hardware/MotorDriver.h": r'''
#pragma once
#include <Arduino.h>
#include "motion/MotionTypes.h"
namespace tongdou {
struct MotorDriver {
  bool fault=false;
  WheelDrive left=WheelDrive::Stop, right=WheelDrive::Stop;
  unsigned starts=0;
  void drive(WheelDrive l, WheelDrive r, uint8_t, uint8_t=0) {
    if(fault) { stop(); return; }
    left=l; right=r; ++starts;
  }
  void stop() { left=right=WheelDrive::Stop; }
  bool faultActive() const { return fault; }
};
}
''',
    "hardware/ImuSensor.h": r'''
#pragma once
#include <Arduino.h>
namespace tongdou {
struct ImuGyroSnapshot { bool ready=true; int32_t zMdegPerSec=0; };
struct ImuSensor {
  bool online=true, readOk=true;
  int32_t rate=0;
  bool ready() const { return online; }
  ImuGyroSnapshot readGyro() { return {online && readOk, rate}; }
};
}
''',
    "hardware/FaceDisplay.h": r'''
#pragma once
#include "face/FaceExpression.h"
namespace tongdou {
struct FaceDisplay {
  FaceExpression expression=FaceExpression::Sleep;
  unsigned writes=0;
  void show(FaceExpression v) { expression=v; ++writes; }
};
}
''',
    "hardware/LedPixel.h": r'''
#pragma once
#include "light/LightTypes.h"
namespace tongdou {
struct LedPixel {
  unsigned writes=0;
  void show(RgbColor) { ++writes; }
  void off() { ++writes; }
};
}
''',
    "hardware/AudioOutput.h": r'''
#pragma once
#include <stdint.h>
namespace tongdou {
struct AudioOutput {
  unsigned plays=0;
  void playClip(uint16_t, const char*) { ++plays; }
};
}
''',
}


def main():
    firmware = Path(__file__).resolve().parents[1]
    compiler = shutil.which("g++") or str(
        Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe")
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
    with tempfile.TemporaryDirectory(prefix="tongdou-motion-test-") as directory:
        work = Path(directory)
        for name, content in STUBS.items():
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content, encoding="utf-8")
        executable = work / "test_voice_motion.exe"
        sources = ["scenario/ScenarioExecutor.cpp", "motion/GyroMotionController.cpp",
                   "motion/MotionPack.cpp", "face/FacePack.cpp", "light/LightPack.cpp",
                   "voice/VoiceLinePack.cpp"]
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                        "-I", str(work), "-I", str(firmware / "src"),
                        str(firmware / "tests/test_voice_motion.cpp"),
                        *[str(firmware / "src" / s) for s in sources],
                        "-o", str(executable)], check=True, env=env)
        subprocess.run([str(executable)], check=True, env=env)


if __name__ == "__main__":
    main()
