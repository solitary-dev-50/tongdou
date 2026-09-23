"""在电脑上编译真实电量计和备用检测代码，用模拟总线验证故障与时序。"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ARDUINO = r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <utility>
#define ARDUINO_ISR_ATTR
#define F(x) x
using gpio_num_t = int;
constexpr int INPUT=1, INPUT_PULLUP=2, HIGH=1, LOW=0, FALLING=3, OUTPUT_OPEN_DRAIN=4;
extern uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
extern int pinModes[49];
extern uint32_t wakeLowAt, wakeReleasedAt;
extern unsigned wakePulseCount;
inline void digitalWrite(int p,int v) {
  if(p!=34 || v!=LOW) __builtin_abort();
}
inline void pinMode(int p,int m) {
  if(p==34 && m==OUTPUT_OPEN_DRAIN) wakeLowAt=fakeMillis;
  if(p==34 && pinModes[p]==OUTPUT_OPEN_DRAIN && m==INPUT) {
    if(fakeMillis-wakeLowAt<2) __builtin_abort();
    wakeReleasedAt=fakeMillis; ++wakePulseCount;
  }
  pinModes[p]=m;
}
inline int digitalRead(int p) { return p==17 ? HIGH : LOW; }
inline int analogRead(int) { return 2000; }
inline uint32_t analogReadMilliVolts(int) { return 1850; }
inline int digitalPinToInterrupt(int p) { return p; }
extern void (*irqHandler)(void*);
extern void* irqContext;
inline void detachInterrupt(int pin) {
  if(pin!=34) __builtin_abort();
  irqHandler=nullptr; irqContext=nullptr;
}
inline void attachInterruptArg(int pin,void (*fn)(void*),void* ctx,int edge) {
  if(pin!=34 || edge!=FALLING) __builtin_abort();
  irqHandler=fn; irqContext=ctx;
}
struct TestSerial {
  template<class... T> void printf(const char*,T...) {}
  template<class... T> void println(T...) {}
};
extern TestSerial Serial;
'''
WIRE = r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <map>
class TwoWire {
 public:
  uint16_t timeout=50;
  bool online=true, badIdentity=false, denyConfig=false, corruptReadback=false;
  bool denyUnseal=false;
  int shortRegister=-1;
  uint16_t security=6, selected=0, previousKey=0;
  bool config=false, entering=false;
  uint32_t enterAt=0;
  unsigned transactions=0, writes=0, capacityWrites=0;
  uint8_t reg=0;
  std::vector<uint8_t> tx,rx,staged;
  size_t rxIndex=0;
  std::map<uint16_t,uint16_t> ram;
  std::map<uint8_t,uint16_t> words;
  std::vector<uint32_t> temperatureReadTimes;
  TwoWire();
  uint16_t getTimeOut() { return timeout; }
  void setTimeOut(uint16_t v) { timeout=v; }
  void beginTransmission(uint8_t address);
  size_t write(uint8_t v) { tx.push_back(v); return 1; }
  size_t write(const uint8_t* b,size_t n) { tx.insert(tx.end(),b,b+n); return n; }
  uint8_t endTransmission(bool stop=true);
  size_t requestFrom(uint8_t address,uint8_t length);
  int read() { return rx.at(rxIndex++); }
};
extern TwoWire Wire;
'''


def main():
    firmware = Path(__file__).resolve().parents[1]
    compiler = shutil.which("g++") or str(
        Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe")
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
    with tempfile.TemporaryDirectory(prefix="tongdou-battery-test-") as directory:
        work = Path(directory)
        pins = "\n".join(f"constexpr int GPIO_NUM_{i}={i};" for i in range(49))
        (work / "Arduino.h").write_text(ARDUINO + pins, encoding="utf-8")
        (work / "Wire.h").write_text(WIRE, encoding="utf-8")
        for revision in (90, 94):
            binary = work / f"battery_{revision}.exe"
            subprocess.run([
                compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                f"-DTONGDOU_BOARD_REVISION={revision}", "-I", str(work),
                "-I", str(firmware / "include"), "-I", str(firmware / "src"),
                str(firmware / "tests/test_bq27220.cpp"),
                str(firmware / "src/hardware/Bq27220.cpp"),
                str(firmware / "src/hardware/BatteryMonitor.cpp"),
                "-o", str(binary),
            ], check=True, env=env)
            subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
