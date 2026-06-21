#pragma once

#include <Arduino.h>

#include "diagnostic/HardwareSelfTestService.h"
#include "hardware/BatteryMonitor.h"
#include "hardware/AudioInput.h"
#include "hardware/AudioOutput.h"
#include "hardware/DisplayBus.h"
#include "hardware/FaceDisplay.h"
#include "hardware/LedPixel.h"
#include "hardware/MotorDriver.h"
#include "hardware/RadarSensor.h"
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

  BatteryMonitor battery_;
  DisplayBus displayBus_;
  FaceDisplay faceDisplay_;
  LedPixel led_;
  MotorDriver motors_;
  RadarSensor radar_;
  AudioInput audioInput_;
  AudioOutput audio_;
  WifiConfigStore wifiConfigStore_;
  TimeService timeService_;
  ReminderStore reminderStore_;
  ReminderManager reminderManager_{timeService_, reminderStore_};
  McpServer mcpServer_{timeService_, reminderStore_};
  ScenarioLibrary scenarioLibrary_;
  ScenarioEngine scenarioEngine_{scenarioLibrary_};
  ScenarioExecutor scenarioExecutor_{faceDisplay_, led_, motors_, audio_};
  ScenarioContext scenarioContext_;
  ScenarioConfigStore scenarioConfigStore_;
  ScenarioConfigApi scenarioConfigApi_{scenarioConfigStore_};
  WebConfigServer webConfigServer_{scenarioConfigApi_, wifiConfigStore_, timeService_,
                                   reminderStore_, mcpServer_};
  HardwareSelfTestService hardwareSelfTest_{battery_, audioInput_, audio_, led_, motors_,
                                           radar_};
  unsigned long lastHeartbeatMs_ = 0;
};

}  // namespace tongdou
