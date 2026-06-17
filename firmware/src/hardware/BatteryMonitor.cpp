#include "hardware/BatteryMonitor.h"

#include "tongdou/Pins.h"

namespace tongdou {

void BatteryMonitor::begin() {
  pinMode(pins::USB_CHG, INPUT);
  pinMode(pins::VBAT_ADC, INPUT);
  pinMode(pins::CHRG, INPUT_PULLUP);
  pinMode(pins::STBY, INPUT_PULLUP);
}

BatterySnapshot BatteryMonitor::read() const {
  BatterySnapshot snapshot;
  snapshot.usbPresent = digitalRead(pins::USB_CHG) == HIGH;
  snapshot.charging = digitalRead(pins::CHRG) == LOW;
  snapshot.standby = digitalRead(pins::STBY) == LOW;
  snapshot.rawAdc = analogRead(pins::VBAT_ADC);
  return snapshot;
}

}  // namespace tongdou
