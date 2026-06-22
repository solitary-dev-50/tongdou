#include "web/WebConfigServer.h"

#include <WiFi.h>

namespace tongdou {
namespace {

constexpr const char* kPortalSsid = "TongDou-Setup";
constexpr uint16_t kDnsPort = 53;
const IPAddress kPortalIp(192, 168, 4, 1);
const IPAddress kPortalGateway(192, 168, 4, 1);
const IPAddress kPortalSubnet(255, 255, 255, 0);
constexpr unsigned long kConnectTimeoutMs = 12000;
constexpr unsigned long kDeferredConnectDelayMs = 400;

String htmlEscape(const String& value) {
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  return escaped;
}

String jsonEscape(const String& value) {
  String escaped = value;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  return escaped;
}

class StringPrint : public Print {
 public:
  size_t write(uint8_t value) override {
    value_ += static_cast<char>(value);
    return 1;
  }

  const String& value() const {
    return value_;
  }

 private:
  String value_;
};

}  // namespace

WebConfigServer::WebConfigServer(ScenarioConfigApi& scenarioApi, WifiConfigStore& wifiStore,
                                 TimeService& timeService, ReminderStore& reminderStore,
                                 McpServer& mcpServer,
                                 HardwareSelfTestService& hardwareSelfTest)
    : scenarioApi_(scenarioApi),
      wifiStore_(wifiStore),
      timeService_(timeService),
      reminderStore_(reminderStore),
      mcpServer_(mcpServer),
      hardwareSelfTest_(hardwareSelfTest) {}

void WebConfigServer::begin() {
  scenarioApi_.begin();
  WifiCredentials credentials;
  if (wifiStore_.load(credentials)) {
    startStation(credentials);
  } else {
    startPortal();
  }
  ensureHttpServer();
}

void WebConfigServer::update() {
  scenarioApi_.update();
  if (dnsStarted_) {
    dnsServer_.processNextRequest();
  }
  if (serverStarted_) {
    server_.handleClient();
  }

  if (pendingStationConnect_ && millis() - stationConnectAfterMs_ >= kDeferredConnectDelayMs) {
    pendingStationConnect_ = false;
    startStation(pendingCredentials_);
  }

  if (state_ == NetworkState::Connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      state_ = NetworkState::Connected;
      timeService_.configureNetworkTime();
      Serial.print("wifi connected ip=");
      Serial.println(WiFi.localIP());
    } else if (millis() - connectStartedMs_ > kConnectTimeoutMs) {
      Serial.println("wifi connect timeout, starting setup portal");
      startPortal();
    }
  }

  timeService_.update();
}

ScenarioConfigApi& WebConfigServer::scenarioApi() {
  return scenarioApi_;
}

void WebConfigServer::startPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.softAPConfig(kPortalIp, kPortalGateway, kPortalSubnet);
  WiFi.softAP(kPortalSsid);

  dnsServer_.start(kDnsPort, "*", kPortalIp);
  dnsStarted_ = true;
  state_ = NetworkState::Portal;

  Serial.print("setup portal started ssid=");
  Serial.print(kPortalSsid);
  Serial.print(" ip=");
  Serial.println(WiFi.softAPIP());
}

void WebConfigServer::startStation(const WifiCredentials& credentials) {
  WiFi.mode(state_ == NetworkState::Portal ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
  connectStartedMs_ = millis();
  state_ = NetworkState::Connecting;

  Serial.print("wifi connecting ssid=");
  Serial.println(credentials.ssid);
}

void WebConfigServer::ensureHttpServer() {
  if (serverStarted_) {
    return;
  }

  setupRoutes();
  server_.begin();
  serverStarted_ = true;
  Serial.println("web config server started");
}

void WebConfigServer::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/status", HTTP_GET, [this]() { sendText(200, "application/json", statusJson()); });
  server_.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
  server_.on("/api/setup/wifi", HTTP_POST, [this]() { handleWifiSave(); });
  server_.on("/api/time/sync", HTTP_POST, [this]() { handleTimeSync(); });
  server_.on("/api/time/status", HTTP_GET, [this]() { handleTimeStatus(); });
  server_.on("/api/reminders", HTTP_GET, [this]() { handleReminderList(); });
  server_.on("/api/reminders", HTTP_POST, [this]() { handleReminderCreate(); });
  server_.on("/api/reminders/delete", HTTP_POST, [this]() { handleReminderDelete(); });
  server_.on("/api/diagnostic", HTTP_POST, [this]() { handleDiagnosticCommand(); });
  server_.on("/api/mcp", HTTP_POST, [this]() { handleMcp(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void WebConfigServer::handleRoot() {
  const String ip = state_ == NetworkState::Connected ? WiFi.localIP().toString()
                                                       : WiFi.softAPIP().toString();
  String body;
  body.reserve(7800);
  body += F("<!doctype html><html><head><meta charset='utf-8'>");
  body += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>铜豆控制台</title><style>");
  body += F("body{font-family:Arial,'Microsoft YaHei',sans-serif;margin:0;background:#121212;color:#f4f1ea}");
  body += F("main{max-width:920px;margin:auto;padding:22px}h1{margin:0 0 6px}.sub{color:#aaa;margin:0 0 18px}");
  body += F("input,button{box-sizing:border-box;width:100%;padding:12px;margin:6px 0;border-radius:8px;border:0}");
  body += F("button{background:#e7b45f;color:#111;font-weight:700;cursor:pointer}button:disabled{background:#555;color:#aaa;cursor:not-allowed}");
  body += F(".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px}.status{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:8px}");
  body += F(".card{background:#1d1d1d;padding:18px;border-radius:10px;margin:14px 0}.item{background:#0f0f0f;border:1px solid #2a2a2a;border-radius:8px;padding:12px}");
  body += F(".label{color:#aaa;font-size:12px}.value{font-size:18px;margin-top:4px}pre{white-space:pre-wrap;background:#0b0b0b;padding:12px;border-radius:8px;min-height:100px}");
  body += F("small{color:#aaa}code{color:#e7b45f}</style></head><body><main>");
  body += F("<h1>铜豆控制台</h1><p class='sub'>配网、校时和硬件诊断。</p>");
  body += F("<div class='card'><h2>当前状态</h2><div class='status'>");
  body += F("<div class='item'><div class='label'>地址</div><div class='value'><code>");
  body += ip;
  body += F("</code></div></div>");
  body += F("<div class='item'><div class='label'>时间</div><div class='value'><code>");
  body += htmlEscape(timeService_.ready() ? timeService_.isoNow() : "未校时");
  body += F("</code></div></div>");
  body += F("<div class='item'><div class='label'>联网</div><div class='value' id='wifi'>读取中</div></div>");
  body += F("<div class='item'><div class='label'>模式</div><div class='value' id='mode'>读取中</div></div>");
  body += F("<div class='item'><div class='label'>硬件</div><div class='value' id='hardware'>读取中</div></div>");
  body += F("<div class='item'><div class='label'>保留能力</div><div class='value' id='capability'>读取中</div></div>");
  body += F("</div><small>第一版需要联网校时，提醒才知道真正日期。</small></div>");
  body += F("<div class='card'><h2>连接 WiFi</h2><form method='post' action='/api/setup/wifi'>");
  body += F("<input name='ssid' placeholder='WiFi 名称' required>");
  body += F("<input name='password' type='password' placeholder='WiFi 密码'>");
  body += F("<button type='submit'>保存并连接</button></form></div>");
  body += F("<div class='card'><h2>时间</h2><button onclick='syncTime()'>用手机/电脑时间同步</button>");
  body += F("<small id='msg'></small></div>");
  body += F("<div class='card'><h2>硬件诊断</h2><p><small>这些按钮会调用固件里的硬件自检服务。</small></p>");
  body += F("<h3>状态读取</h3><div class='grid'>");
  body += F("<button onclick=\"diag('selftest')\">完整自检</button>");
  body += F("<button onclick=\"diag('status')\">硬件状态</button>");
  body += F("<button onclick=\"diag('battery')\">电池/充电</button>");
  body += F("<button onclick=\"diag('radar')\">雷达</button>");
  body += F("<button onclick=\"diag('mic')\">麦克风</button>");
  body += F("</div><h3>输出测试</h3><div class='grid'>");
  body += F("<button onclick=\"diag('speaker')\">喇叭测试音</button>");
  body += F("<button onclick=\"diag('led red')\">红灯</button>");
  body += F("<button onclick=\"diag('led green')\">绿灯</button>");
  body += F("<button onclick=\"diag('led blue')\">蓝灯</button>");
  body += F("<button onclick=\"diag('led off')\">关灯</button>");
  body += F("<button onclick=\"diag('motor forward')\">电机正转</button>");
  body += F("<button onclick=\"diag('motor reverse')\">电机反转</button>");
  body += F("<button onclick=\"diag('motor stop')\">电机停止</button>");
  body += F("</div><h3>剧情测试</h3><div class='grid'>");
  body += F("<button disabled>剧情触发测试后续接入</button>");
  body += F("</div><pre id='diag'>等待诊断命令。</pre></div>");
  body += F("<script>");
  body += F("function modeText(m){return {normal:'正常',portal:'配网',connecting:'联网中',degraded:'降级'}[m]||m;}");
  body += F("function capText(c){let a=[];if(c.reminders)a.push('提醒');if(c.web)a.push('网页');if(c.visual)a.push('视觉');if(c.sound)a.push('声音');if(c.voiceInput)a.push('语音输入');return a.join(' / ')||'无';}");
  body += F("async function loadStatus(){try{let r=await fetch('/api/status');let j=await r.json();document.getElementById('wifi').textContent=j.wifiConnected?'已连接':(j.portal?'配网模式':'连接中');document.getElementById('mode').textContent=modeText(j.systemMode);document.getElementById('hardware').textContent=j.hardware.degraded?'降级':'正常';document.getElementById('capability').textContent=capText(j.capabilities);}catch(e){document.getElementById('wifi').textContent='读取失败';document.getElementById('mode').textContent='读取失败';document.getElementById('hardware').textContent='读取失败';document.getElementById('capability').textContent='读取失败';}}");
  body += F("async function syncTime(){let e=Math.floor(Date.now()/1000);let r=await fetch('/api/time/sync',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'epoch='+e});document.getElementById('msg').textContent=await r.text();}");
  body += F("async function diag(c){let o=document.getElementById('diag');o.textContent='执行中：'+c;try{let r=await fetch('/api/diagnostic',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'command='+encodeURIComponent(c)});o.textContent=await r.text();}catch(e){o.textContent='诊断失败：'+e;}}");
  body += F("loadStatus();");
  body += F("</script>");
  body += F("</main></body></html>");
  sendText(200, "text/html; charset=utf-8", body);
}

void WebConfigServer::handleWifiSave() {
  WifiCredentials credentials;
  credentials.ssid = server_.arg("ssid");
  credentials.password = server_.arg("password");
  credentials.ssid.trim();
  if (credentials.ssid.length() == 0) {
    sendText(400, "text/plain; charset=utf-8", "缺少 WiFi 名称");
    return;
  }

  wifiStore_.save(credentials);
  pendingCredentials_ = credentials;
  pendingStationConnect_ = true;
  stationConnectAfterMs_ = millis();
  sendText(200, "text/plain; charset=utf-8", "已保存，铜豆马上连接 WiFi。");
}

void WebConfigServer::handleWifiScan() {
  const int count = WiFi.scanNetworks();
  String body = "[";
  for (int i = 0; i < count; ++i) {
    if (i > 0) {
      body += ",";
    }
    body += "{\"ssid\":\"";
    body += jsonEscape(WiFi.SSID(i));
    body += "\",\"rssi\":";
    body += WiFi.RSSI(i);
    body += "}";
  }
  body += "]";
  sendText(200, "application/json", body);
}

void WebConfigServer::handleTimeSync() {
  const uint32_t epoch = static_cast<uint32_t>(server_.arg("epoch").toInt());
  if (!timeService_.setEpoch(epoch)) {
    sendText(400, "text/plain; charset=utf-8", "时间无效");
    return;
  }

  sendText(200, "text/plain; charset=utf-8", "时间已同步：" + timeService_.isoNow());
}

void WebConfigServer::handleTimeStatus() {
  sendText(200, "application/json", statusJson());
}

void WebConfigServer::handleReminderList() {
  sendText(200, "application/json", remindersJson());
}

void WebConfigServer::handleReminderCreate() {
  const uint32_t dueAt = static_cast<uint32_t>(server_.arg("dueAt").toInt());
  String text = server_.arg("text");
  text.trim();

  if (dueAt == 0 || text.length() == 0) {
    sendText(400, "text/plain; charset=utf-8", "missing dueAt or text");
    return;
  }

  if (timeService_.ready() && dueAt <= static_cast<uint32_t>(timeService_.now())) {
    sendText(400, "text/plain; charset=utf-8", "dueAt is in the past");
    return;
  }

  ReminderRecord created;
  if (!reminderStore_.add(dueAt, text, &created)) {
    sendText(507, "text/plain; charset=utf-8", "reminder storage is full");
    return;
  }

  String body;
  body.reserve(160);
  body += "{\"ok\":true,\"id\":";
  body += created.id;
  body += ",\"dueAt\":";
  body += created.dueAt;
  body += "}";
  sendText(200, "application/json", body);
}

void WebConfigServer::handleReminderDelete() {
  const uint32_t id = static_cast<uint32_t>(server_.arg("id").toInt());
  if (id == 0 || !reminderStore_.remove(id)) {
    sendText(404, "text/plain; charset=utf-8", "reminder not found");
    return;
  }

  sendText(200, "application/json", "{\"ok\":true}");
}

void WebConfigServer::handleDiagnosticCommand() {
  String command = server_.arg("command");
  command.trim();
  if (command.length() == 0) {
    sendText(400, "text/plain; charset=utf-8", "missing command");
    return;
  }

  StringPrint output;
  if (!hardwareSelfTest_.handleCommand(command, output)) {
    sendText(404, "text/plain; charset=utf-8", "unknown diagnostic command");
    return;
  }

  sendText(200, "text/plain; charset=utf-8", output.value());
}

void WebConfigServer::handleMcp() {
  const String payload = server_.arg("plain");
  if (payload.length() == 0) {
    sendText(400, "application/json",
             "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"message\":\"empty payload\"}}");
    return;
  }

  sendText(200, "application/json", mcpServer_.handlePayloadText(payload));
}

void WebConfigServer::handleNotFound() {
  if (state_ == NetworkState::Portal) {
    server_.sendHeader("Location", String("http://") + kPortalIp.toString() + "/", true);
    server_.send(302, "text/plain", "");
    return;
  }

  sendText(404, "text/plain; charset=utf-8", "Not found");
}

void WebConfigServer::sendText(int code, const char* type, const String& body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(code, type, body);
}

String WebConfigServer::statusJson() const {
  const HardwareDiagnosticStatus hardware = hardwareSelfTest_.status();
  String body;
  body.reserve(560);
  const char* systemMode = "normal";
  if (state_ == NetworkState::Portal) {
    systemMode = "portal";
  } else if (state_ == NetworkState::Connecting) {
    systemMode = "connecting";
  } else if (hardware.degraded) {
    systemMode = "degraded";
  }

  body += "{\"wifiConnected\":";
  body += WiFi.status() == WL_CONNECTED ? "true" : "false";
  body += ",\"portal\":";
  body += state_ == NetworkState::Portal ? "true" : "false";
  body += ",\"timeReady\":";
  body += timeService_.ready() ? "true" : "false";
  body += ",\"now\":\"";
  body += timeService_.ready() ? timeService_.isoNow() : "";
  body += "\",\"systemMode\":\"";
  body += systemMode;
  body += "\",\"hardware\":{\"face\":";
  body += hardware.faceReady ? "true" : "false";
  body += ",\"led\":";
  body += hardware.ledReady ? "true" : "false";
  body += ",\"audioInput\":";
  body += hardware.audioInputReady ? "true" : "false";
  body += ",\"audioOutput\":";
  body += hardware.audioOutputReady ? "true" : "false";
  body += ",\"degraded\":";
  body += hardware.degraded ? "true" : "false";
  body += "},\"capabilities\":{\"reminders\":";
  body += hardware.remindersAvailable ? "true" : "false";
  body += ",\"web\":";
  body += hardware.webAvailable ? "true" : "false";
  body += ",\"visual\":";
  body += hardware.visualFeedbackAvailable ? "true" : "false";
  body += ",\"sound\":";
  body += hardware.soundAvailable ? "true" : "false";
  body += ",\"voiceInput\":";
  body += hardware.voiceInputAvailable ? "true" : "false";
  body += "}}";
  return body;
}

String WebConfigServer::remindersJson() const {
  String body = "[";
  const ReminderRecord* records = reminderStore_.records();
  bool first = true;
  for (uint8_t i = 0; i < reminderStore_.capacity(); ++i) {
    const ReminderRecord& record = records[i];
    if (!record.active) {
      continue;
    }

    if (!first) {
      body += ",";
    }
    first = false;
    body += "{\"id\":";
    body += record.id;
    body += ",\"dueAt\":";
    body += record.dueAt;
    body += ",\"completed\":";
    body += record.completed ? "true" : "false";
    body += ",\"text\":\"";
    body += jsonEscape(String(record.text));
    body += "\"}";
  }
  body += "]";
  return body;
}

}  // namespace tongdou
