#include "hardware/BatteryMonitor.h"

#include "tongdou/Pins.h"

namespace tongdou {
namespace {

constexpr uint16_t kAdcReferenceMv = 3300;
constexpr uint8_t kAdcBits = 12;
constexpr uint16_t kAdcMax = (1U << kAdcBits) - 1U;
constexpr uint8_t kBatteryDividerMultiplier = 2;
constexpr uint16_t kBatteryEmptyMv = 3300;
constexpr uint16_t kBatteryFullMv = 4200;

uint8_t estimateBatteryPercent(uint16_t voltageMv) {
  if (voltageMv <= kBatteryEmptyMv) {
    return 0;
  }
  if (voltageMv >= kBatteryFullMv) {
    return 100;
  }

  const uint32_t span = kBatteryFullMv - kBatteryEmptyMv;
  const uint32_t aboveEmpty = voltageMv - kBatteryEmptyMv;
  return static_cast<uint8_t>((aboveEmpty * 100U + span / 2U) / span);
}

uint16_t rawAdcToBatteryMv(int rawAdc) {
  const uint32_t pinMv =
      (static_cast<uint32_t>(constrain(rawAdc, 0, static_cast<int>(kAdcMax))) *
       kAdcReferenceMv + kAdcMax / 2U) /
      kAdcMax;
  return static_cast<uint16_t>(pinMv * kBatteryDividerMultiplier);
}

}  // namespace

void BatteryMonitor::begin() {
  analogReadResolution(kAdcBits);
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
  snapshot.voltageMv = rawAdcToBatteryMv(snapshot.rawAdc);
  snapshot.percent = estimateBatteryPercent(snapshot.voltageMv);
  return snapshot;
}

}  // namespace tongdou
