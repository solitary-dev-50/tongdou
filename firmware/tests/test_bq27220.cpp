#include <cassert>
#include <cstdio>
#include "hardware/BatteryMonitor.h"
#include "tongdou/Pins.h"

uint32_t fakeMillis = 0;
uint32_t wakeLowAt = 0, wakeReleasedAt = 0;
unsigned wakePulseCount = 0;
int pinModes[49] = {};
void (*irqHandler)(void*) = nullptr;
void* irqContext = nullptr;
TestSerial Serial;
TwoWire Wire;

TwoWire::TwoWire() {
  ram={{0x929D,3000},{0x9206,0x0484},{0x9208,0x1000},{0x920D,0},
       {0x920B,1},{0x91FD,4200},{0x9221,10},{0x929F,3000}};
  words={{0x06,2981},{0x08,3890},{0x0A,8},{0x0C,static_cast<uint16_t>(-123)},
         {0x10,600},{0x12,1200},{0x14,static_cast<uint16_t>(-100)},
         {0x2C,50},{0x2E,100}};
}
void TwoWire::beginTransmission(uint8_t address) {
  assert(address==0x55);
  assert(wakePulseCount>0 && fakeMillis-wakeReleasedAt>=300);
  assert(timeout==10);
  tx.clear();
}
uint8_t TwoWire::endTransmission(bool stop) {
  reg=tx.at(0);
  if(!stop) return online ? 0 : 2;
  ++transactions;
  if(!online) return 2;
  ++writes;
  assert(tx.size()>=3);
  const uint16_t value=tx[1] | (static_cast<uint16_t>(tx[2])<<8);
  if(reg==0x60) {
    assert(config && security==2);
    assert(tx.size()==3 && tx[2]==staged.size()+2);
    uint8_t sum=0;
    for(uint8_t b:staged) sum+=b;
    assert(tx[1]==static_cast<uint8_t>(255-sum));
    const uint16_t address=staged[0] | (static_cast<uint16_t>(staged[1])<<8);
    // 确认没有写入 OTP、校准或不在本任务范围内的地址。
    assert(ram.count(address));
    uint16_t parameter=staged.size()==3 ? staged[2] : (staged[2]<<8)|staged[3];
    if(!corruptReadback) ram[address]=parameter;
    if(address==0x929D) ++capacityWrites;
    return 0;
  }
  assert(reg==0x00 || reg==0x3E);
  if(reg==0x3E && tx.size()>3) {
    assert(config && security==2);
    staged.assign(tx.begin()+1,tx.end());
    return 0;
  }
  if(value>=0x9100 && value<=0x9300) { selected=value; return 0; }
  switch(value) {
    case 1: selected=1; break;
    case 0x0414: previousKey=value; break;
    case 0x3672:
      if(previousKey==0x0414 && !denyUnseal) security=4;
      previousKey=0; break;
    case 0xFFFF:
      if(previousKey==0xFFFF && security!=6) security=2;
      previousKey=value; break;
    case 0x0090:
      assert(security==2);
      enterAt=fakeMillis; entering=true; break;
    case 0x0091: case 0x0092:
      config=false; entering=false; break;
    case 0x0030: security=6; break;
    case 0x000D: words[0x0A]|=8; break;
    default: assert(false && "unexpected control or OTP command");
  }
  return 0;
}
size_t TwoWire::requestFrom(uint8_t address,uint8_t length) {
  assert(address==0x55 && timeout==10);
  ++transactions;
  rx.clear(); rxIndex=0;
  if(!online) return 0;
  if(entering && !denyConfig && fakeMillis-enterAt>=2000) config=true;
  if(reg==0x3E) {
    rx={static_cast<uint8_t>(selected),static_cast<uint8_t>(selected>>8)};
    if(selected==1) { rx.push_back(0x20); rx.push_back(badIdentity ? 3 : 2); }
    else {
      assert(security==2 && ram.count(selected));
      const uint16_t value=ram[selected];
      if(length==4) rx.push_back(value>>8);
      rx.push_back(value&0xFF);
    }
  } else {
    uint16_t value=reg==0x3A ? (security|0x20|(config?0x0400:0)) :
                   reg==0x3C ? ram[0x929F] : words.at(reg);
    rx={static_cast<uint8_t>(value),static_cast<uint8_t>(value>>8)};
    if(reg==0x06) temperatureReadTimes.push_back(fakeMillis);
  }
  assert(rx.size()==length);
  if(shortRegister==reg) rx.pop_back();
  return rx.size();
}

#if TONGDOU_BOARD_REVISION == 94
void tick(tongdou::BatteryMonitor& battery,uint32_t duration) {
  for(uint32_t elapsed=0;elapsed<duration;elapsed+=10) {
    const unsigned before=Wire.transactions;
    battery.update();
    assert(Wire.transactions-before<=1); // 每轮最多一笔短读写，不在循环里等芯片。
    assert(Wire.timeout==50); // 共享总线原超时恢复。
    fakeMillis+=10;
  }
}
void resetBus() { Wire=TwoWire(); fakeMillis=0; wakePulseCount=0; }
void ready(tongdou::BatteryMonitor& battery) {
  battery.begin(); tick(battery,8000); assert(battery.read().usingFuelGauge);
}

int main() {
  using tongdou::BatteryMonitor;
  static_assert(tongdou::pins::BQ27220_GPOUT==34, "gauge interrupt pin");
  static_assert(tongdou::pins::QMI8658_INT1==33, "IMU interrupt one pin");
  static_assert(tongdou::pins::QMI8658_INT2==47, "IMU interrupt two pin");
  {
    resetBus(); BatteryMonitor b; ready(b);
    const auto s=b.read();
    assert(wakePulseCount==1);
    assert(s.voltageMv==3890 && s.adcVoltageMv==3700);
    assert(s.gauge.currentMa==-123 && s.gauge.averageCurrentMa==-100);
    assert(s.gauge.stateOfChargePercent==50 && s.gauge.remainingCapacityMah==600);
    assert(s.gauge.fullChargeCapacityMah==1200 && s.gauge.stateOfHealthPercent==100);
    assert(s.gauge.temperatureCentiC==2495);
    assert(Wire.ram[0x929F]==1200 && Wire.ram[0x929D]==1200);
    assert(Wire.ram[0x91FD]==4200 && Wire.ram[0x9221]==0);
    assert((Wire.ram[0x9208]&0x62)==0x60 && (Wire.ram[0x920D]&3)==0);
    assert(Wire.security==6 && !Wire.config && pinModes[34]==INPUT);
    // ISR 不访问总线，短脉冲触发采样，电量变化和低电量分别记录。
    const auto calls=Wire.transactions;
    irqHandler(irqContext); assert(Wire.transactions==calls);
    Wire.words[0x2C]=9; Wire.words[0x0C]=50;
    tick(b,700);
    assert(b.read().gauge.lowBattery && b.read().gauge.lowBatteryEventCount==1);
    assert(b.read().gauge.socChangeCount==1 && b.read().gauge.currentMa==50);
    for(int i=0;i<30;++i) { irqHandler(irqContext); tick(b,20); }
    for(size_t i=1;i<Wire.temperatureReadTimes.size();++i)
      assert(Wire.temperatureReadTimes[i]-Wire.temperatureReadTimes[i-1]>=500);
    // 不完整的一帧立即回退，不能继续冒用上次正常值。
    Wire.shortRegister=0x14; tick(b,1000);
    assert(!b.read().usingFuelGauge && b.read().voltageMv==3700);
    assert(b.read().usbPresent && b.read().charging);
    Wire.shortRegister=-1; Wire.words[0x12]=1000; Wire.ram[0x929D]=1000;
    const unsigned capacityWrites=Wire.capacityWrites;
    tick(b,38000);
    assert(wakePulseCount>=2);
    assert(b.read().usingFuelGauge && Wire.capacityWrites==capacityWrites);
    assert(Wire.ram[0x929D]==1000); // 重连不重置学习结果。
    fakeMillis+=6000;
    assert(!b.read().usingFuelGauge); // 缓存超过期限不可继续使用。
    tick(b,1000); assert(b.read().usingFuelGauge);
    Wire.ram[0x929F]=3000; tick(b,1000);
    assert(!b.read().usingFuelGauge); // 电量计自身掉电丢配置。
    tick(b,40000); assert(b.read().usingFuelGauge && Wire.ram[0x929F]==1200);
  }
  {
    resetBus(); Wire.online=false; BatteryMonitor b; b.begin(); tick(b,1000);
    assert(!b.read().usingFuelGauge && b.read().voltageMv==3700);
    const unsigned calls=Wire.transactions; tick(b,10000);
    assert(Wire.transactions==calls); // 断线不持续占用总线。
    Wire.online=true; tick(b,40000); assert(b.read().usingFuelGauge);
  }
  for(int fault=0;fault<4;++fault) {
    resetBus();
    Wire.badIdentity=fault==0; Wire.denyConfig=fault==1;
    Wire.corruptReadback=fault==2; Wire.denyUnseal=fault==3;
    BatteryMonitor b; b.begin(); tick(b,18000);
    assert(!b.read().usingFuelGauge && b.read().voltageMv==3700);
    assert(!Wire.config); // 失败后清理配置状态。
  }
  {
    resetBus(); fakeMillis=0xFFFFF000u; BatteryMonitor b; ready(b);
    assert(b.read().usingFuelGauge); // 毫秒计数回绕。
  }
  std::puts("V9.4 battery driver: all communication, RAM, event, fallback and timing checks passed");
}
#else
int main() {
  tongdou::BatteryMonitor battery;
  battery.begin(); battery.update();
  assert(!battery.read().usingFuelGauge && battery.read().voltageMv==3700);
  assert(Wire.transactions==0 && irqHandler==nullptr);
  assert(pinModes[34]==0 && pinModes[47]==0);
  std::puts("V9.0: ADC preserved; no gauge transactions or new interrupt pin ownership");
}
#endif
