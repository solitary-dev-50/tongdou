#pragma once

#include <Arduino.h>

#include "hardware/HardwareManager.h"
#include "mcp/McpServer.h"
#include "reminder/ReminderManager.h"
#include "reminder/ReminderStore.h"
#include "scenario/ScenarioEngine.h"
#include "scenario/ScenarioExecutor.h"
#include "scenario/ScenarioLibrary.h"
#include "system/TimeService.h"
#include "system/WifiConfigStore.h"
#include "web/ScenarioConfigApi.h"
#include "web/ScenarioConfigStore.h"
#include "web/WebConfigServer.h"

namespace tongdou {

class App {
 public:
  void begin();
  void update();

 private:
  void handleReminderEvents();
  void handleSerialDiagnostics();
  void logHeartbeat();

  HardwareManager hardware_;
  WifiConfigStore wifiConfigStore_;
  TimeService timeService_;
  ReminderStore reminderStore_;
  ReminderManager reminderManager_{timeService_, reminderStore_};
  McpServer mcpServer_{timeService_, reminderStore_};
  ScenarioLibrary scenarioLibrary_;
  ScenarioEngine scenarioEngine_{scenarioLibrary_};
  ScenarioExecutor scenarioExecutor_{hardware_.faceDisplay(), hardware_.led(),
                                    hardware_.motors(), hardware_.audioOutput()};
  ScenarioContext scenarioContext_;
  ScenarioConfigStore scenarioConfigStore_;
  ScenarioConfigApi scenarioConfigApi_{scenarioConfigStore_};
  WebConfigServer webConfigServer_{scenarioConfigApi_, wifiConfigStore_, timeService_,
                                   reminderStore_, mcpServer_, hardware_.selfTest()};
  unsigned long lastHeartbeatMs_ = 0;
};

}  // namespace tongdou
