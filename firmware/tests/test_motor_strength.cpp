#include <cassert>
#include <iostream>
#include <Arduino.h>
#include <Preferences.h>
#include "hardware/MotorDriver.h"

unsigned long fakeMillis = 0;
TestSerial Serial;
unsigned pwmDuty[4] = {};
int pinLevel[64] = {};
std::map<std::string,unsigned> savedMotorPreferences;
using namespace tongdou;

int main() {
  MotorDriver motors;
  motors.begin();
  motors.saveCalibration(false, false, 190, 160);
  motors.driveAtPercent(WheelDrive::Forward, WheelDrive::Reverse, 50);
  assert(pwmDuty[0] == 95 && pwmDuty[1] == 0);
  assert(pwmDuty[2] == 0 && pwmDuty[3] == 80);

  // 轻微动作不能修改正常力度。
  motors.drive(WheelDrive::Forward, WheelDrive::Reverse);
  assert(pwmDuty[0] == 190 && pwmDuty[3] == 160);

  // 用户修改并保存正常力度后，下一次轻微动作和重新初始化后均跟随取半。
  motors.saveCalibration(false, false, 120, 200);
  motors.driveAtPercent(WheelDrive::Reverse, WheelDrive::Forward, 50);
  assert(pwmDuty[0] == 0 && pwmDuty[1] == 60);
  assert(pwmDuty[2] == 100 && pwmDuty[3] == 0);
  MotorDriver restored;
  restored.begin();
  assert(restored.leftDefaultDuty() == 120 && restored.rightDefaultDuty() == 200);
  restored.driveAtPercent(WheelDrive::Forward, WheelDrive::Reverse, 50);
  assert(pwmDuty[0] == 60 && pwmDuty[3] == 100);
  restored.driveAtPercent(WheelDrive::Forward, WheelDrive::Forward, 0);
  for (const auto duty : pwmDuty) assert(duty == 0);
  std::cout << "motor strength: half of saved calibration, normal power and stop passed\n";
}
