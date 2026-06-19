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

}  // namespace

WebConfigServer::WebConfigServer(ScenarioConfigApi& scenarioApi, WifiConfigStore& wifiStore,
                                 TimeService& timeService)
    : scenarioApi_(scenarioApi), wifiStore_(wifiStore), timeService_(timeService) {}

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
  server_.onNotFound([this]() { handleNotFound(); });
}

void WebConfigServer::handleRoot() {
  const String ip = state_ == NetworkState::Connected ? WiFi.localIP().toString()
                                                       : WiFi.softAPIP().toString();
  String body;
  body.reserve(2600);
  body += F("<!doctype html><html><head><meta charset='utf-8'>");
  body += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>铜豆配网</title><style>");
  body += F("body{font-family:Arial,'Microsoft YaHei',sans-serif;margin:24px;background:#111;color:#f4f1ea}");
  body += F("main{max-width:560px;margin:auto}input,button{box-sizing:border-box;width:100%;padding:12px;margin:8px 0;border-radius:8px;border:0}");
  body += F("button{background:#e7b45f;color:#111;font-weight:700}.card{background:#1d1d1d;padding:18px;border-radius:10px;margin:14px 0}");
  body += F("small{color:#aaa}code{color:#e7b45f}</style></head><body><main>");
  body += F("<h1>铜豆配网</h1><div class='card'><p>当前地址：<code>");
  body += ip;
  body += F("</code></p><p>当前时间：<code>");
  body += htmlEscape(timeService_.ready() ? timeService_.isoNow() : "未校时");
  body += F("</code></p><small>第一版需要联网校时，提醒才知道真正日期。</small></div>");
  body += F("<div class='card'><h2>连接 WiFi</h2><form method='post' action='/api/setup/wifi'>");
  body += F("<input name='ssid' placeholder='WiFi 名称' required>");
  body += F("<input name='password' type='password' placeholder='WiFi 密码'>");
  body += F("<button type='submit'>保存并连接</button></form></div>");
  body += F("<div class='card'><h2>时间</h2><button onclick='syncTime()'>用手机/电脑时间同步</button>");
  body += F("<small id='msg'></small></div>");
  body += F("<script>async function syncTime(){let e=Math.floor(Date.now()/1000);let r=await fetch('/api/time/sync',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'epoch='+e});document.getElementById('msg').textContent=await r.text();}</script>");
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
  String body;
  body.reserve(220);
  body += "{\"wifiConnected\":";
  body += WiFi.status() == WL_CONNECTED ? "true" : "false";
  body += ",\"portal\":";
  body += state_ == NetworkState::Portal ? "true" : "false";
  body += ",\"timeReady\":";
  body += timeService_.ready() ? "true" : "false";
  body += ",\"now\":\"";
  body += timeService_.ready() ? timeService_.isoNow() : "";
  body += "\"}";
  return body;
}

}  // namespace tongdou
