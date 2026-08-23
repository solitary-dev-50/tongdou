#pragma once

#include <DNSServer.h>
#include <WebServer.h>

#include "diagnostic/HardwareSelfTestService.h"
#include "mcp/McpServer.h"
#include "reminder/ReminderStore.h"
#include "system/DeviceLocaleStore.h"
#include "system/TimeService.h"
#include "system/WifiConfigStore.h"
#include "web/ScenarioConfigApi.h"
#include "web/WebApiContract.h"

namespace tongdou {

enum class WebConfigEvent : uint8_t {
  None,
  SetupPortalStarted,
  LocaleChanged,
  WifiConnected,
  WifiConnectFailed,
};

class WebConfigServer {
 public:
  WebConfigServer(ScenarioConfigApi& scenarioApi, WifiConfigStore& wifiStore,
                  TimeService& timeService, ReminderStore& reminderStore, McpServer& mcpServer,
                  HardwareSelfTestService& hardwareSelfTest,
                  DeviceLocaleStore& deviceLocaleStore);

  void begin();
  void update();
  ScenarioConfigApi& scenarioApi();
  WebConfigEvent consumeEvent();

 private:
  enum class NetworkState : uint8_t {
    Idle,
    Connecting,
    Connected,
    Portal,
  };

  void startPortal();
  void startStation(const WifiCredentials& credentials);
  void ensureHttpServer();
  void setupRoutes();
  void handleRoot();
  void handleWifiSave();
  void handleWifiScan();
  void handleLocaleGet();
  void handleLocaleSave();
  void handleTimeSync();
  void handleTimeStatus();
  void handleReminderList();
  void handleReminderCreate();
  void handleReminderDelete();
  void handleDiagnosticCommand();
  void handleMcp();
  void handleVoiceDebugPage();
  void handleVoiceStatus();
  void handleVoiceBackendConfigure();
  void handleVoiceBackendConnect();
  void handleVoiceDetect();
  void handleVoiceStart();
  void handleVoiceAbort();
  void handleVoiceCodecSelfTest();
  void handleNotFound();
  void sendText(int code, const char* type, const String& body);
  String callMcpTool(const char* name, const String& argumentsJson);
  String statusJson() const;
  String remindersJson() const;

  ScenarioConfigApi& scenarioApi_;
  WifiConfigStore& wifiStore_;
  TimeService& timeService_;
  ReminderStore& reminderStore_;
  McpServer& mcpServer_;
  HardwareSelfTestService& hardwareSelfTest_;
  DeviceLocaleStore& deviceLocaleStore_;
  WebServer server_{80};
  DNSServer dnsServer_;
  NetworkState state_ = NetworkState::Idle;
  bool serverStarted_ = false;
  bool dnsStarted_ = false;
  bool pendingStationConnect_ = false;
  bool setupPortalStartedEvent_ = false;
  bool localeChangedEvent_ = false;
  bool wifiConnectedEvent_ = false;
  bool wifiConnectFailedEvent_ = false;
  WifiCredentials pendingCredentials_;
  unsigned long connectStartedMs_ = 0;
  unsigned long stationConnectAfterMs_ = 0;
};

}  // namespace tongdou
