"""编译真实屏幕代码，检查连接图标、电池保留和专用页面保护。"""
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
#include <string>
#include <algorithm>
#include <cstdlib>
using String = std::string;
using std::min;
using std::max;
extern uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
struct TestSerial { template<class T> void println(T) {} };
extern TestSerial Serial;
''',
    "Wire.h": r'''
#pragma once
#include <array>
#include <vector>
#include <stdint.h>
struct TestWire {
  std::array<uint8_t,1024> screen={};
  std::vector<uint8_t> tx;
  unsigned offset=0, writes=0;
  void beginTransmission(uint8_t) { tx.clear(); }
  void write(uint8_t value) { tx.push_back(value); }
  int endTransmission() {
    if(tx.size()>1 && tx[0]==0x40) {
      ++writes;
      for(size_t i=1;i<tx.size();++i) screen[offset++%1024]=tx[i];
    }
    return 0;
  }
};
extern TestWire Wire;
''',
    "hardware/BatteryMonitor.h": r'''
#pragma once
#include <stdint.h>
namespace tongdou {
struct BatterySnapshot {
  bool usingFuelGauge=true, charging=false;
  uint16_t voltageMv=3800;
  struct { uint16_t stateOfChargePercent=65; } gauge;
};
struct BatteryMonitor {
  BatterySnapshot value;
  BatterySnapshot read() const { return value; }
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
    with tempfile.TemporaryDirectory(prefix="tongdou-status-test-") as directory:
        work = Path(directory)
        for name, contents in STUBS.items():
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(contents, encoding="utf-8")
        executable = work / "test_status_bar.exe"
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                        "-I", str(work), "-I", str(firmware / "src"),
                        str(firmware / "src/hardware/FaceDisplay.cpp"),
                        str(firmware / "tests/test_status_bar.cpp"),
                        "-o", str(executable)], check=True, env=env)
        subprocess.run([str(executable)], check=True, env=env)

if __name__ == "__main__":
    main()
