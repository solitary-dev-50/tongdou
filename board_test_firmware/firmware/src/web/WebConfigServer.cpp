#include "web/WebConfigServer.h"

#include <WiFi.h>

namespace tongdou {
namespace {

constexpr const char* kPortalSsid = "TongDou-BoardTest";
constexpr uint16_t kDnsPort = 53;
const IPAddress kPortalIp(192, 168, 4, 1);
const IPAddress kPortalGateway(192, 168, 4, 1);
const IPAddress kPortalSubnet(255, 255, 255, 0);

uint8_t parseDutyArg(const String& value, uint8_t fallback) {
  if (value.length() == 0) {
    return fallback;
  }

  char* end = nullptr;
  const long parsed = strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) {
    return fallback;
  }

  return static_cast<uint8_t>(constrain(parsed, 0, 255));
}

String boolJson(bool value) {
  return value ? "true" : "false";
}

}  // namespace

WebConfigServer::WebConfigServer(HardwareSelfTestService& hardwareSelfTest)
    : hardwareSelfTest_(hardwareSelfTest) {}

void WebConfigServer::begin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(kPortalIp, kPortalGateway, kPortalSubnet);
  WiFi.softAP(kPortalSsid);

  dnsServer_.start(kDnsPort, "*", kPortalIp);
  dnsStarted_ = true;

  setupRoutes();
  server_.begin();
  serverStarted_ = true;

  Serial.print(F("board test ap ssid="));
  Serial.print(kPortalSsid);
  Serial.print(F(" ip="));
  Serial.println(WiFi.softAPIP());
}

void WebConfigServer::update() {
  if (dnsStarted_) {
    dnsServer_.processNextRequest();
  }
  if (serverStarted_) {
    server_.handleClient();
  }
}

void WebConfigServer::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/motor", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/diagnostic", HTTP_POST, [this]() { handleDiagnosticCommand(); });
  server_.on("/api/motor/config", HTTP_GET, [this]() { handleMotorConfigGet(); });
  server_.on("/api/motor/config", HTTP_POST, [this]() { handleMotorConfigSave(); });
  server_.on("/generate_204", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/gen_204", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/hotspot-detect.html", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/connecttest.txt", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/redirect", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/favicon.ico", HTTP_GET,
             [this]() { sendText(204, "image/x-icon", ""); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void WebConfigServer::handleBoardTestPage() {
  static const char kPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TongDou Board Test</title>
<style>
:root{font-family:Arial,Helvetica,sans-serif;color:#202124;background:#f6f7f8}
body{margin:0;padding:16px;overflow-y:auto}
main{max-width:820px;margin:0 auto}
h1{font-size:24px;margin:0 0 4px}
h2{font-size:17px;margin:22px 0 8px}
p{margin:6px 0;color:#5f6368}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(132px,1fr));gap:8px}
button{min-height:42px;border:1px solid #c9ced6;background:#fff;border-radius:8px;font-size:15px}
button:active{transform:translateY(1px)}
.primary{background:#174ea6;color:#fff;border-color:#174ea6}
.danger{background:#b3261e;color:#fff;border-color:#b3261e}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px;align-items:center}
label{display:flex;gap:8px;align-items:center;margin:8px 0}
input[type=number],input[type=text]{width:100%;box-sizing:border-box;padding:9px;border:1px solid #c9ced6;border-radius:6px;font-size:15px}
pre{white-space:pre-wrap;word-break:break-word;background:#111;color:#d9fdd3;border-radius:8px;padding:12px;min-height:160px}
.small{font-size:13px;color:#5f6368}
</style>
</head>
<body>
<main>
<h1>TongDou V9 Board Test</h1>
<p>AP: TongDou-BoardTest · Page: 192.168.4.1/motor</p>

<h2>Status</h2>
<div class="grid">
<button onclick="refreshStatus()">Refresh</button>
<button onclick="copyLog()">Copy log</button>
</div>
<pre id="status">loading...</pre>

<h2>Quick Checks</h2>
<div class="grid">
<button onclick="diag('selftest')">Self Test</button>
<button onclick="diag('battery')">Battery</button>
<button onclick="diag('mic')">Mic</button>
<button onclick="diag('speaker')">Speaker</button>
<button onclick="diag('i2c scan')">I2C Scan</button>
<button onclick="diag('imu')">IMU</button>
<button onclick="diag('imu raw test')">IMU Raw Test</button>
<button onclick="diag('radar')">Radar</button>
</div>

<h2>LED</h2>
<div class="grid">
<button onclick="diag('led red')">Red</button>
<button onclick="diag('led green')">Green</button>
<button onclick="diag('led blue')">Blue</button>
<button onclick="diag('led off')">Off</button>
</div>

<h2>Motor Direction And Power</h2>
<div class="row">
<label><input id="leftInv" type="checkbox"> Left reversed</label>
<label><input id="rightInv" type="checkbox"> Right reversed</label>
</div>
<div class="row">
<label>Left PWM <input id="leftPwm" type="number" min="0" max="255"></label>
<label>Right PWM <input id="rightPwm" type="number" min="0" max="255"></label>
</div>
<div class="grid">
<button class="primary" onclick="saveMotorConfig()">Save motor setup</button>
<button class="danger" onclick="diag('motor stop')">Stop</button>
<button onclick="diag('motor forward')">Pulse forward</button>
<button onclick="diag('motor reverse')">Pulse reverse</button>
<button onclick="manualMotor('forward')">Manual forward</button>
<button onclick="manualMotor('reverse')">Manual reverse</button>
<button onclick="gyroStraight()">Gyro auto straight</button>
</div>
<p class="small">Manual motor commands time out automatically. Use Stop before lifting the board.</p>

<h2>Serial Style Command</h2>
<div class="row">
<input id="cmd" type="text" placeholder="example: audio volume 60">
<button onclick="diag(document.getElementById('cmd').value)">Run</button>
</div>

<h2>Log</h2>
<pre id="log"></pre>
</main>
<script>
const $=id=>document.getElementById(id);
function append(text){$('log').textContent=(text||'')+'\n\n'+$('log').textContent}
async function post(path, body){
  const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  return await r.text();
}
async function diag(command){
  if(!command)return;
  const text=await post('/api/diagnostic','command='+encodeURIComponent(command));
  append('$ '+command+'\n'+text);
  refreshStatus();
}
async function refreshStatus(){
  const r=await fetch('/api/status');
  $('status').textContent=JSON.stringify(await r.json(),null,2);
}
async function loadMotorConfig(){
  const r=await fetch('/api/motor/config');
  const j=await r.json();
  $('leftInv').checked=j.leftInverted;
  $('rightInv').checked=j.rightInverted;
  $('leftPwm').value=j.leftPwm;
  $('rightPwm').value=j.rightPwm;
}
async function saveMotorConfig(){
  const body='leftInverted='+($('leftInv').checked?'1':'0')+
    '&rightInverted='+($('rightInv').checked?'1':'0')+
    '&leftPwm='+encodeURIComponent($('leftPwm').value)+
    '&rightPwm='+encodeURIComponent($('rightPwm').value);
  append(await post('/api/motor/config',body));
  await loadMotorConfig();
  refreshStatus();
}
function manualMotor(direction){
  diag('motor manual '+direction+' '+$('leftPwm').value+' '+$('rightPwm').value);
}
async function gyroStraight(){
  await diag('motor auto forward '+$('leftPwm').value);
  setTimeout(()=>diag('motor auto status'),2600);
}
async function copyLog(){
  const text='STATUS\n'+$('status').textContent+'\n\nLOG\n'+$('log').textContent;
  await navigator.clipboard.writeText(text);
}
window.addEventListener('beforeunload',()=>navigator.sendBeacon('/api/diagnostic','command=motor%20stop'));
loadMotorConfig();
refreshStatus();
</script>
</body>
</html>
)HTML";

  sendText(200, "text/html; charset=utf-8", FPSTR(kPage));
}

void WebConfigServer::handleMotorConfigGet() {
  String body = "{";
  body += "\"leftInverted\":";
  body += boolJson(hardwareSelfTest_.motorLeftInverted());
  body += ",\"rightInverted\":";
  body += boolJson(hardwareSelfTest_.motorRightInverted());
  body += ",\"leftPwm\":";
  body += hardwareSelfTest_.motorLeftDefaultDuty();
  body += ",\"rightPwm\":";
  body += hardwareSelfTest_.motorRightDefaultDuty();
  body += "}";
  sendText(200, "application/json", body);
}

void WebConfigServer::handleMotorConfigSave() {
  const bool leftInverted = server_.arg("leftInverted") == "1";
  const bool rightInverted = server_.arg("rightInverted") == "1";
  const uint8_t leftDuty =
      parseDutyArg(server_.arg("leftPwm"), hardwareSelfTest_.motorLeftDefaultDuty());
  const uint8_t rightDuty =
      parseDutyArg(server_.arg("rightPwm"), hardwareSelfTest_.motorRightDefaultDuty());

  hardwareSelfTest_.saveMotorCalibration(leftInverted, rightInverted, leftDuty,
                                         rightDuty);
  sendText(200, "application/json", "{\"ok\":true}");
}

void WebConfigServer::handleDiagnosticCommand() {
  String command = server_.arg("command");
  if (command.length() == 0 && server_.hasArg("plain")) {
    command = server_.arg("plain");
  }
  command.trim();
  if (command.length() == 0) {
    sendText(400, "text/plain", "missing command");
    return;
  }

  String output;
  class StringPrint : public Print {
   public:
    explicit StringPrint(String& value) : value_(value) {}
    size_t write(uint8_t c) override {
      value_ += static_cast<char>(c);
      return 1;
    }

   private:
    String& value_;
  } out(output);

  if (!hardwareSelfTest_.handleCommand(command, out)) {
    output = "unknown command, run help";
  }
  sendText(200, "text/plain; charset=utf-8", output);
}

void WebConfigServer::handleStatus() {
  sendText(200, "application/json", statusJson());
}

void WebConfigServer::handleNotFound() {
  handleBoardTestPage();
}

void WebConfigServer::sendText(int code, const char* type, const String& body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(code, type, body);
}

String WebConfigServer::statusJson() const {
  const HardwareDiagnosticStatus status = hardwareSelfTest_.status();
  const BatterySnapshot battery = hardwareSelfTest_.batterySnapshot();

  String body = "{";
  body += "\"faceReady\":";
  body += boolJson(status.faceReady);
  body += ",\"ledReady\":";
  body += boolJson(status.ledReady);
  body += ",\"audioInputReady\":";
  body += boolJson(status.audioInputReady);
  body += ",\"audioOutputReady\":";
  body += boolJson(status.audioOutputReady);
  body += ",\"degraded\":";
  body += boolJson(status.degraded);
  body += ",\"usbPresent\":";
  body += boolJson(battery.usbPresent);
  body += ",\"charging\":";
  body += boolJson(battery.charging);
  body += ",\"standby\":";
  body += boolJson(battery.standby);
  body += ",\"rawAdc\":";
  body += battery.rawAdc;
  body += ",\"audioVolumePercent\":";
  body += hardwareSelfTest_.audioVolumePercent();
  body += ",\"motorLeftInverted\":";
  body += boolJson(hardwareSelfTest_.motorLeftInverted());
  body += ",\"motorRightInverted\":";
  body += boolJson(hardwareSelfTest_.motorRightInverted());
  body += ",\"motorLeftPwm\":";
  body += hardwareSelfTest_.motorLeftDefaultDuty();
  body += ",\"motorRightPwm\":";
  body += hardwareSelfTest_.motorRightDefaultDuty();
  body += "}";
  return body;
}

}  // namespace tongdou
