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

  hardware_.begin();
  scenarioExecutor_.begin();
  wifiConfigStore_.begin();
  timeService_.begin();
  reminderStore_.begin();
  reminderManager_.begin();
  mcpServer_.begin();
  scenarioConfigStore_.begin();
  webConfigServer_.begin();

  const ScenarioPlan bootPlan =
      scenarioEngine_.select({ScenarioEventType::BootCompleted, 0}, scenarioContext_);
  scenarioExecutor_.execute(bootPlan);

  Serial.println("hardware skeleton initialized");
}

void App::update() {
  webConfigServer_.update();
  hardware_.update();
  handleSerialDiagnostics();
  handleReminderEvents();
  scenarioExecutor_.update();
  logHeartbeat();
}

void App::handleReminderEvents() {
  ScenarioEvent event;
  if (!reminderManager_.update(event)) {
    return;
  }

  const ScenarioPlan plan = scenarioEngine_.select(event, scenarioContext_);
  scenarioExecutor_.execute(plan);
}

void App::handleSerialDiagnostics() {
  if (!Serial.available()) {
    return;
  }

  const String command = Serial.readStringUntil('\n');
  if (hardware_.selfTest().handleCommand(command, Serial)) {
    return;
  }

  Serial.println("unknown command, type help");
}

void App::logHeartbeat() {
  const unsigned long now = millis();
  if (now - lastHeartbeatMs_ < kHeartbeatMs) {
    return;
  }

  lastHeartbeatMs_ = now;
  const BatterySnapshot battery = hardware_.battery().read();

  Serial.print("heartbeat usb=");
  Serial.print(battery.usbPresent ? "1" : "0");
  Serial.print(" chrg=");
  Serial.print(battery.charging ? "1" : "0");
  Serial.print(" stby=");
  Serial.print(battery.standby ? "1" : "0");
  Serial.print(" vbat_adc=");
  Serial.print(battery.rawAdc);
  Serial.print(" time=");
  Serial.println(timeService_.ready() ? timeService_.isoNow() : "not-ready");
}

}  // namespace tongdou
