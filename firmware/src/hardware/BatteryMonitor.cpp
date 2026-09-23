#include "hardware/BatteryMonitor.h"

#include "tongdou/Pins.h"
#include "tongdou/BatteryConfig.h"

namespace tongdou {

void BatteryMonitor::begin() {
  pinMode(pins::USB_CHG, INPUT);
  pinMode(pins::VBAT_ADC, INPUT);
  pinMode(pins::CHRG, INPUT_PULLUP);
  pinMode(pins::STBY, INPUT_PULLUP);
  sampleAdc();
#if TONGDOU_BOARD_REVISION == 94
  gauge_.begin();
#endif
  Serial.printf("battery: source=adc voltage=%umV raw=%d\n", adcVoltageMv_, rawAdc_);
}

void BatteryMonitor::sampleAdc() {
  rawAdc_ = analogRead(pins::VBAT_ADC);
  adcVoltageMv_ = static_cast<uint16_t>(
      analogReadMilliVolts(pins::VBAT_ADC) * battery_config::kAdcDividerRatio);
  lastAdcAtMs_ = static_cast<uint32_t>(millis());
}

void BatteryMonitor::update() {
  if (static_cast<uint32_t>(millis()) - lastAdcAtMs_ >= 500) sampleAdc();
#if TONGDOU_BOARD_REVISION == 94
  gauge_.update();
  const bool usingGauge = gauge_.snapshot().valid;
  if (usingGauge != wasUsingFuelGauge_) {
    wasUsingFuelGauge_ = usingGauge;
    Serial.printf("battery: source=%s adc_voltage=%umV reason=%s\n",
                  usingGauge ? "bq27220" : "adc", adcVoltageMv_, gauge_.lastError());
  }
#endif
}

BatterySnapshot BatteryMonitor::read() const {
  BatterySnapshot snapshot;
  snapshot.usbPresent = digitalRead(pins::USB_CHG) == HIGH;
  snapshot.charging = digitalRead(pins::CHRG) == LOW;
  snapshot.standby = digitalRead(pins::STBY) == LOW;
  // 网页、诊断和升级判断只取缓存；USB/充电状态仍实时读取原有硬件脚。
  snapshot.rawAdc = rawAdc_;
  snapshot.adcVoltageMv = adcVoltageMv_;
#if TONGDOU_BOARD_REVISION == 94
  snapshot.gauge = gauge_.snapshot();
  snapshot.gaugeError = gauge_.lastError();
#endif
  snapshot.usingFuelGauge = snapshot.gauge.valid;
  snapshot.voltageMv = snapshot.usingFuelGauge ? snapshot.gauge.voltageMv : adcVoltageMv_;
  return snapshot;
}

}  // namespace tongdou
