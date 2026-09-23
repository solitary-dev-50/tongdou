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
  assert(pixel(33,10) && !pixel(35,7)); // 气泡内是勾，不是叉或三点。
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
  assert(pixel(30,7) && pixel(35,7) && pixel(40,7));
  display.setConnectionStatus(true,-60,false,false,false);
  refresh();
  assert(pixel(35,7) && !pixel(30,7)); // 服务离线，保留无线信号。
  display.setConnectionStatus(false,-40,true,true,false); // 断网压过旧的服务快照。
  refresh();
  assert(pixel(9,8) && pixel(35,7));

  // 电池更新不能擦掉连接图标。
  display.setConnectionStatus(true,-70,true,true,false);
  refresh();
  const auto connections=Wire.screen;
  battery.value.charging=true;
  refresh();
  for(int page=0;page<2;++page)
    for(int x=0;x<46;++x) assert(Wire.screen[page*128+x]==connections[page*128+x]);
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
  assert(pixel(9,8) && pixel(35,7));
  std::cout << "status bar: connection changes, battery preservation and page protection passed\n";
}
