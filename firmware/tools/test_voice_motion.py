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
  uint8_t lastDuty=255, lastPercent=0;
  void drive(WheelDrive l, WheelDrive r, uint8_t duty, uint8_t=0) {
    if(fault) { stop(); return; }
    lastDuty=duty;
    lastPercent=0;
    left=l; right=r; ++starts;
  }
  void stop() { left=right=WheelDrive::Stop; }
  void driveAtPercent(WheelDrive l, WheelDrive r, uint8_t percent) {
    drive(l,r,0);
    lastPercent=percent;
  }
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

MOTOR_STUBS = {
    "Arduino.h": STUBS["Arduino.h"] + r'''
using gpio_num_t = int;
using std::min;
constexpr int LOW=0, HIGH=1, INPUT_PULLUP=2, OUTPUT=3;
extern unsigned pwmDuty[4];
extern int pinLevel[64];
inline void digitalWrite(int pin, int level) { pinLevel[pin]=level; }
inline int digitalRead(int) { return HIGH; }
inline void pinMode(int,int) {}
inline void ledcSetup(uint8_t,uint32_t,uint8_t) {}
inline void ledcAttachPin(int,uint8_t) {}
inline void ledcWrite(uint8_t channel,uint8_t duty) { pwmDuty[channel]=duty; }
inline void delayMicroseconds(unsigned) {}
''',
    "Preferences.h": r'''
#pragma once
#include <map>
#include <string>
#include <stdint.h>
extern std::map<std::string,unsigned> savedMotorPreferences;
class Preferences {
public:
  bool begin(const char*,bool) { return true; }
  bool getBool(const char* key,bool fallback) { return savedMotorPreferences.count(key)?savedMotorPreferences[key]:fallback; }
  uint8_t getUChar(const char* key,uint8_t fallback) { return savedMotorPreferences.count(key)?savedMotorPreferences[key]:fallback; }
  void putBool(const char* key,bool value) { savedMotorPreferences[key]=value; }
  void putUChar(const char* key,uint8_t value) { savedMotorPreferences[key]=value; }
  void end() {}
};
''',
    "tongdou/Pins.h": r'''
#pragma once
namespace tongdou { namespace pins {
constexpr int MOTOR_FAULT=37, MOTOR_SLEEP=42;
constexpr int MOTOR_AIN1=1, MOTOR_AIN2=2, MOTOR_BIN1=3, MOTOR_BIN2=4;
} }
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
                   "voice/VoiceLinePack.cpp", "scenario/ScenarioEngine.cpp",
                   "scenario/ScenarioLibrary.cpp"]
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                        "-I", str(work), "-I", str(firmware / "src"),
                        str(firmware / "tests/test_voice_motion.cpp"),
                        *[str(firmware / "src" / s) for s in sources],
                        "-o", str(executable)], check=True, env=env)
        subprocess.run([str(executable)], check=True, env=env)

        motor_work = work / "motor_strength"
        for name, content in MOTOR_STUBS.items():
            target = motor_work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content, encoding="utf-8")
        motor_executable = motor_work / "test_motor_strength.exe"
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                        "-I", str(motor_work), "-I", str(firmware / "src"),
                        str(firmware / "tests/test_motor_strength.cpp"),
                        str(firmware / "src/hardware/MotorDriver.cpp"),
                        "-o", str(motor_executable)], check=True, env=env)
        subprocess.run([str(motor_executable)], check=True, env=env)


if __name__ == "__main__":
    main()
