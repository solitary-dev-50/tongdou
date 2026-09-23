#include <cassert>
#include <iostream>
#include "scenario/ScenarioExecutor.h"

unsigned long fakeMillis = 100;
TestSerial Serial;
using namespace tongdou;

int main() {
  FaceDisplay face;
  LedPixel led;
  MotorDriver motors;
  AudioOutput audio;
  ImuSensor imu;
  ScenarioExecutor executor(face, led, motors, audio, imu);
  executor.begin();
  auto tick = [&](unsigned long ms) { fakeMillis += ms; executor.update(); };
  auto stopped = [&]() {
    assert(!executor.motionRunning());
    assert(motors.left == WheelDrive::Stop && motors.right == WheelDrive::Stop);
  };

  // 三种动作使用真实帧：方向正确，时间到自动停止。
  for (auto action : {MotionAction::NudgeForward, MotionAction::ShrinkBack,
                      MotionAction::TinyShake}) {
    assert(executor.startUserMotion(action));
    assert(!executor.startUserMotion(action)); // 忙时不重启、不排队。
    tick(180);
    if (action == MotionAction::NudgeForward) {
      assert(motors.left == WheelDrive::Forward && motors.right == WheelDrive::Forward);
      tick(100);
    } else if (action == MotionAction::ShrinkBack) {
      assert(motors.left == WheelDrive::Reverse && motors.right == WheelDrive::Reverse);
      tick(120);
    } else {
      assert(motors.left == WheelDrive::Forward && motors.right == WheelDrive::Reverse);
      tick(70);
      assert(motors.left == WheelDrive::Reverse && motors.right == WheelDrive::Forward);
      tick(70);
    }
    tick(1);
    stopped();
  }

  // 停止覆盖准备、运动、回正阶段，后续更新不得再启动。
  for (int phase = 0; phase < 3; ++phase) {
    imu.rate = 0;
    assert(executor.startUserMotion(MotionAction::NudgeForward));
    if (phase >= 1) tick(180);
    if (phase == 2) {
      imu.rate = 100000;
      tick(100);
      tick(1);
      assert(executor.motionRunning()); // 此时进入回正。
    }
    const auto starts = motors.starts;
    executor.stopMotion();
    stopped();
    for (int i=0; i<20; ++i) tick(100);
    stopped();
    assert(motors.starts == starts);
  }
  executor.stopMotion(); // 重复停止仍成功。
  stopped();

  // 故障和陀螺仪缺失拒绝启动；运行中读失败必须终止。
  imu.rate=0;
  motors.fault=true;
  assert(!executor.startUserMotion(MotionAction::NudgeForward));
  stopped();
  motors.fault=false;
  imu.online=false;
  assert(!executor.startUserMotion(MotionAction::ShrinkBack));
  stopped();
  imu.online=true;
  assert(executor.startUserMotion(MotionAction::TinyShake));
  tick(180);
  imu.readOk=false;
  tick(1);
  stopped();
  imu.readOk=true;
  assert(executor.startUserMotion(MotionAction::TinyShake));
  tick(180);
  motors.fault=true;
  tick(1);
  stopped();
  motors.fault=false;

  // 新的提醒剧情可以接管；语音停止只停电机，不清屏、不熄灯、不改声音。
  assert(executor.startUserMotion(MotionAction::NudgeForward));
  ScenarioPlan reminder;
  reminder.valid=true;
  reminder.face=FaceAction::Serious;
  reminder.light=LightAction::SoftWhite;
  reminder.motion=MotionAction::None;
  executor.execute(reminder);
  stopped();
  const auto faceWrites=face.writes, ledWrites=led.writes, plays=audio.plays;
  assert(executor.startUserMotion(MotionAction::TinyShake));
  executor.stopMotion();
  assert(face.writes==faceWrites && led.writes==ledWrites && audio.plays==plays);
  tick(1000); // 原有表情仍可继续推进，不应被停止命令重置成睡觉。
  assert(face.expression != FaceExpression::Sleep);
  assert(led.writes==ledWrites && audio.plays==plays);
  stopped();
  std::cout << "voice motion: direction, deadline, busy, stop, fault and reminder tests passed\n";
}
