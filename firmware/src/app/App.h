#pragma once

#include <Arduino.h>

#include "app/AppCommandService.h"
#include "hardware/HardwareManager.h"
#include "mcp/McpServer.h"
#include "realtime/RealtimeVoiceService.h"
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
  void handleRealtimeVoiceEvents();
  void handleSerialDiagnostics();
  void handleStartupReport();
  void printStartupReport(const char* title);

  HardwareManager hardware_;
  WifiConfigStore wifiConfigStore_;
  TimeService timeService_;
  ReminderStore reminderStore_;
  ReminderManager reminderManager_{timeService_, reminderStore_};
  ScenarioLibrary scenarioLibrary_;
  ScenarioEngine scenarioEngine_{scenarioLibrary_};
  ScenarioExecutor scenarioExecutor_{hardware_.faceDisplay(), hardware_.led(),
                                    hardware_.motors(), hardware_.audioOutput()};
  ScenarioContext scenarioContext_;
  RealtimeVoiceService realtimeVoiceService_{hardware_.audioInput(),
                                            hardware_.audioOutput()};
  AppCommandService appCommandService_{scenarioEngine_, scenarioExecutor_, scenarioContext_,
                                      realtimeVoiceService_};
  McpServer mcpServer_{timeService_, reminderStore_, hardware_.selfTest(), appCommandService_};
  ScenarioConfigStore scenarioConfigStore_;
  ScenarioConfigApi scenarioConfigApi_{scenarioConfigStore_};
  WebConfigServer webConfigServer_{scenarioConfigApi_, wifiConfigStore_, timeService_,
                                   reminderStore_, mcpServer_, hardware_.selfTest()};
  unsigned long bootMs_ = 0;
  uint8_t startupReportCount_ = 0;
};

}  // namespace tongdou
