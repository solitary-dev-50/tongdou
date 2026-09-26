#include <cassert>
#include <iostream>
#include <Wire.h>
#include "hardware/FaceDisplay.h"

uint32_t fakeMillis=0;
TestSerial Serial;
TestWire Wire;

int main() {
  tongdou::BatteryMonitor battery;
  tongdou::FaceDisplay display(battery);
  display.begin();
  auto refresh = [&]() { fakeMillis+=1000; display.updateStatusBar(); };
  auto pixel = [&](int x, int y) { return (Wire.screen[x+(y/8)*128] & (1<<(y%8)))!=0; };
  const auto original=Wire.screen;
  // 电池未变化也必须刷新连接图标。
  display.setConnectionStatus(true,-50,true,true,false);
  refresh();
  assert(pixel(18,2)); // 四格强信号。
  assert(pixel(35,10) && !pixel(36,8)); // 云朵内是勾，不是叉或三点。
  for(int page=0;page<2;++page)
    for(int x=84;x<128;++x) assert(Wire.screen[page*128+x]==original[page*128+x]);
  for(int i=256;i<1024;++i) assert(Wire.screen[i]==original[i]);

  // 同一信号等级不无谓刷新；变化按一秒节流。
  unsigned writes=Wire.writes;
  display.setConnectionStatus(true,-54,true,true,false);
  refresh();
  assert(Wire.writes==writes);
  display.setConnectionStatus(true,-80,false,false,true);
  display.updateStatusBar();
  assert(Wire.writes==writes);
  refresh();
  assert(!pixel(18,2) && pixel(3,12));
  assert(pixel(32,8) && pixel(36,8) && pixel(40,8));
  display.setConnectionStatus(true,-60,false,false,false);
  refresh();
  assert(pixel(37,8) && !pixel(32,8)); // 服务离线，保留无线信号。
  display.setConnectionStatus(false,-40,true,true,false); // 断网压过旧的服务快照。
  refresh();
  assert(pixel(9,8) && pixel(37,8));

  // 电池更新不能擦掉连接图标。
  display.setConnectionStatus(true,-70,true,true,false);
  refresh();
  const auto connections=Wire.screen;
  battery.value.charging=true;
  refresh();
  for(int page=0;page<2;++page)
    for(int x=0;x<47;++x) assert(Wire.screen[page*128+x]==connections[page*128+x]);

  // 百分比不再占据数字区域；分格边界和满电不溢出外框。
  battery.value.charging=false;
  const struct { uint16_t percent; int bars; } levels[] = {
      {11,1}, {20,1}, {21,2}, {40,2}, {41,3}, {60,3},
      {61,4}, {80,4}, {81,5}, {99,5}, {100,5}};
  for (const auto& level : levels) {
    battery.value.gauge.stateOfChargePercent=level.percent;
    refresh();
    for(int i=0;i<5;++i) {
      assert(pixel(103+i*4,5)==(i<level.bars));
      assert(!pixel(106+i*4,5)); // 每格之间留白。
    }
    for(int y=0;y<16;++y)
      for(int x=84;x<101;++x) assert(!pixel(x,y));
    assert(pixel(101,2) && pixel(123,13) && pixel(125,7));
  }
  writes=Wire.writes;
  ++battery.value.voltageMv; // 电压不显示，不因此刷新屏幕。
  refresh();
  assert(Wire.writes==writes);

  // 即使数据不变，0%、5%、10% 都能持续闪；换表情不能重置闪烁相位。
  for (uint16_t percent : {0,5,10}) {
    battery.value.gauge.stateOfChargePercent=percent;
    fakeMillis=(fakeMillis/2000+1)*2000;
    display.updateStatusBar();
    assert(pixel(103,5) && !pixel(107,5));
    writes=Wire.writes;
    fakeMillis+=500;
    display.updateStatusBar();
    assert(Wire.writes==writes);
    fakeMillis+=500;
    display.updateStatusBar();
    assert(!pixel(103,5) && pixel(101,2));
    display.show(tongdou::FaceExpression::Smile);
    assert(!pixel(103,5));
    refresh();
    assert(pixel(103,5));
  }
  battery.value.charging=true;
  // 充电只让当前待充格跳动，已经充满的整格始终可见。
  const struct { uint16_t percent; int solidBars; } chargingLevels[] = {
      {0,0}, {5,0}, {19,0}, {20,1}, {39,1}, {40,2}, {45,2},
      {60,3}, {65,3}, {80,4}, {85,4}, {99,4}, {100,5}};
  for (const auto& level : chargingLevels) {
    battery.value.gauge.stateOfChargePercent=level.percent;
    fakeMillis=(fakeMillis/2000+1)*2000;
    display.updateStatusBar();
    for(int i=0;i<5;++i)
      assert(pixel(103+i*4,5)==(i<=level.solidBars));
    refresh();
    for(int i=0;i<5;++i)
      assert(pixel(103+i*4,5)==(i<level.solidBars));
    display.show(tongdou::FaceExpression::Smile);
    for(int i=0;i<5;++i)
      assert(pixel(103+i*4,5)==(i<level.solidBars));
  }
  writes=Wire.writes;
  refresh();
  assert(Wire.writes==writes); // 100% 五格常亮，不再刷新动画。
  battery.value.usingFuelGauge=false;
  refresh();
  assert(pixel(112,10) && pixel(111,4) && !pixel(103,5)); // 无电量不伪造进度。
  writes=Wire.writes;
  refresh();
  assert(Wire.writes==writes);
  battery.value.charging=false;
  refresh();
  assert(pixel(112,10) && pixel(111,4) && !pixel(103,5)); // 问号。
  writes=Wire.writes;
  refresh();
  assert(Wire.writes==writes);
  battery.value.usingFuelGauge=true;
  battery.value.gauge.stateOfChargePercent=45;
  refresh();
  assert(pixel(103,5) && pixel(107,5) && pixel(111,5) && !pixel(115,5));
  writes=Wire.writes;
  refresh();
  assert(Wire.writes==writes); // 拔掉充电后恢复三格静态电量。
  battery.value.charging=true;
  refresh();

  // 专用页面禁止状态栏覆盖，回到表情后恢复最新状态。
  uint8_t bitmap[768];
  std::fill(std::begin(bitmap),std::end(bitmap),0xA5);
  assert(display.showBitmap128x48(bitmap,sizeof(bitmap)));
  auto page=Wire.screen;
  writes=Wire.writes;
  display.setConnectionStatus(false,0,false,false,false);
  refresh();
  assert(Wire.writes==writes && Wire.screen==page);
  display.showActivationCode("123456");
  page=Wire.screen;
  refresh();
  assert(Wire.screen==page);
  display.show(tongdou::FaceExpression::Smile);
  assert(pixel(9,8) && pixel(37,8));
  assert(pixel(103,5) && pixel(107,5));
  assert(pixel(111,5)==((fakeMillis/1000)%2==0));
  std::cout << "status bar: connections, battery segments, low-battery blinking, charging, unknown state and page protection passed\n";
}
