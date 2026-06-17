#include "app/App.h"

#include "tongdou/BuildInfo.h"

namespace tongdou {

void App::begin() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.print(kProjectName);
  Serial.print(" firmware stage: ");
  Serial.println(kFirmwareStage);

  battery_.begin();
  displayBus_.begin();
  led_.begin();
  motors_.begin();
  radar_.begin();
  voice_.begin();

  Serial.println("hardware skeleton initialized");
}

void App::update() {
  logHeartbeat();
}

void App::logHeartbeat() {
  const unsigned long now = millis();
  if (now - lastHeartbeatMs_ < kHeartbeatMs) {
    return;
  }

  lastHeartbeatMs_ = now;
  const BatterySnapshot battery = battery_.read();

  Serial.print("heartbeat usb=");
  Serial.print(battery.usbPresent ? "1" : "0");
  Serial.print(" chrg=");
  Serial.print(battery.charging ? "1" : "0");
  Serial.print(" stby=");
  Serial.print(battery.standby ? "1" : "0");
  Serial.print(" vbat_adc=");
  Serial.println(battery.rawAdc);
}

}  // namespace tongdou
