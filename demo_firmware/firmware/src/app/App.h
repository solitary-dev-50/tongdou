#pragma once

#include <Arduino.h>

#include "app/AppCommandService.h"
#include "demo/DemoScenePlayer.h"
#include "hardware/HardwareManager.h"
#include "mcp/McpServer.h"
#include "motion/GyroReturnWiggleController.h"
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
  void handleLogoTouchEvents();
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
  GyroReturnWiggleController logoWiggle_{hardware_.motors(), hardware_.imu()};
  ScenarioContext scenarioContext_;
  RealtimeVoiceService realtimeVoiceService_{hardware_.audioInput(),
                                            hardware_.audioOutput()};
  DemoScenePlayer demoScenePlayer_{scenarioExecutor_, realtimeVoiceService_};
  AppCommandService appCommandService_{scenarioEngine_, scenarioExecutor_, scenarioContext_,
                                      realtimeVoiceService_};
  McpServer mcpServer_{timeService_, reminderStore_, hardware_.selfTest(), appCommandService_};
  ScenarioConfigStore scenarioConfigStore_;
  ScenarioConfigApi scenarioConfigApi_{scenarioConfigStore_};
  WebConfigServer webConfigServer_{scenarioConfigApi_, wifiConfigStore_, timeService_,
                                   reminderStore_, mcpServer_, hardware_.selfTest(),
                                   demoScenePlayer_};
  unsigned long bootMs_ = 0;
  uint8_t startupReportCount_ = 0;
  bool logoSleepMode_ = false;
};

}  // namespace tongdou
