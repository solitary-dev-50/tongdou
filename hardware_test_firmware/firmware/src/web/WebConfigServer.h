#pragma once

#include <DNSServer.h>
#include <WebServer.h>

#include "diagnostic/HardwareSelfTestService.h"

namespace tongdou {

class WebConfigServer {
 public:
  explicit WebConfigServer(HardwareSelfTestService& hardwareSelfTest);

  void begin();
  void update();

 private:
  void setupRoutes();
  void handleBoardTestPage();
  void handleDiagnosticCommand();
  void handleAudioRecordingDownload();
  void handleRadarStatus();
  void handleStatus();
  void handleNotFound();
  void sendText(int code, const char* type, const String& body);
  String statusJson() const;

  HardwareSelfTestService& hardwareSelfTest_;
  WebServer server_{80};
  DNSServer dnsServer_;
  bool serverStarted_ = false;
  bool dnsStarted_ = false;
};

}  // namespace tongdou
