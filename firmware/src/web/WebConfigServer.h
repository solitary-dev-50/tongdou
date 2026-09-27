#pragma once

#include <DNSServer.h>
#include <WebServer.h>

#include "diagnostic/HardwareSelfTestService.h"
#include "mcp/McpServer.h"
#include "reminder/ReminderStore.h"
#include "resources/LocaleResourceUpdateService.h"
#include "system/DeviceLocaleStore.h"
#include "system/ServerEndpointStore.h"
#include "system/ProactiveReminderConfigStore.h"
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
                  DeviceLocaleStore& deviceLocaleStore,
                  LocaleResourceUpdateService& localeResourceUpdateService,
                  ServerEndpointStore& serverEndpointStore,
                  ProactiveReminderConfigStore& proactiveReminderConfigStore);

  void begin();
  void update();
  ScenarioConfigApi& scenarioApi();
  WebConfigEvent consumeEvent();
  const char* setupSsid() const;

 private:
  enum class NetworkState : uint8_t {
    Idle,
    Connecting,
    Connected,
    Portal,
  };

  void startPortal();
  void startStation(const WifiCredentials& credentials);
  void prepareSavedNetworks();
  bool startNextSavedNetwork();
  void startMdns();
  void stopMdns();
  void ensureHttpServer();
  void setupRoutes();
  void handleRoot();
  void handleWifiSave();
  void handleWifiScan();
  void handleWifiList();
  void handleWifiDelete();
  void handleLocaleGet();
  void handleLocaleSave();
  void handleTimeSync();
  void handleTimeStatus();
  void handleReminderList();
  void handleReminderCreate();
  void handleReminderDelete();
  void handleProactiveReminderGet();
  void handleProactiveReminderSave();
  void handleMotorConfigGet();
  void handleMotorConfigSave();
  void handleDiagnosticCommand();
  void handleMcp();
  void handleVoiceDebugPage();
  void handleVoiceStatus();
  void handleVoiceBackendConfigure();
  void handleVoiceBackendConnect();
  void handleVoiceDetect();
  void handleVoiceStart();
  void handleVoiceStartAuto();
  void handleVoiceAbort();
  void handleVoiceExit();
  void handleVoiceCodecSelfTest();
  void handleNotFound();
  void sendText(int code, const char* type, const String& body);
  String callMcpTool(const char* name, const String& argumentsJson);
  String statusJson() const;
  String remindersJson() const;
  String proactiveRemindersJson() const;

  ScenarioConfigApi& scenarioApi_;
  WifiConfigStore& wifiStore_;
  TimeService& timeService_;
  ReminderStore& reminderStore_;
  McpServer& mcpServer_;
  HardwareSelfTestService& hardwareSelfTest_;
  DeviceLocaleStore& deviceLocaleStore_;
  LocaleResourceUpdateService& localeResourceUpdateService_;
  ServerEndpointStore& serverEndpointStore_;
  ProactiveReminderConfigStore& proactiveReminderConfigStore_;
  WebServer server_{80};
  DNSServer dnsServer_;
  NetworkState state_ = NetworkState::Idle;
  bool serverStarted_ = false;
  bool dnsStarted_ = false;
  bool mdnsStarted_ = false;
  bool pendingStationConnect_ = false;
  bool setupPortalStartedEvent_ = false;
  bool localeChangedEvent_ = false;
  bool wifiConnectedEvent_ = false;
  bool wifiConnectFailedEvent_ = false;
  WifiCredentials pendingCredentials_;
  String connectingSsid_;
  uint8_t savedOrder_[WifiConfigStore::kMaxNetworks] = {};
  uint8_t savedOrderCount_ = 0;
  uint8_t visibleNetworks_ = 0;
  uint8_t nextSavedAttempt_ = 0;
  uint8_t activeNetworkIndex_ = WifiConfigStore::kMaxNetworks;
  bool tryingSavedNetworks_ = false;
  unsigned long connectStartedMs_ = 0;
  unsigned long connectTimeoutMs_ = 0;
  unsigned long stationConnectAfterMs_ = 0;
};

}  // namespace tongdou
