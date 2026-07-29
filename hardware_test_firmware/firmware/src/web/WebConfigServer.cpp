#include "web/WebConfigServer.h"

#include <WiFi.h>

namespace tongdou {
namespace {

constexpr const char* kPortalSsid = "TongDou-BoardTest";
constexpr uint16_t kDnsPort = 53;
const IPAddress kPortalIp(192, 168, 4, 1);
const IPAddress kPortalGateway(192, 168, 4, 1);
const IPAddress kPortalSubnet(255, 255, 255, 0);

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
  server_.on("/api/radar", HTTP_GET, [this]() { handleRadarStatus(); });
  server_.on("/api/diagnostic", HTTP_POST, [this]() { handleDiagnosticCommand(); });
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
body{margin:0;padding:16px;overflow-y:auto;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}
main{max-width:820px;margin:0 auto}
h1{font-size:24px;margin:0 0 4px}
h2{font-size:17px;margin:22px 0 8px}
p{margin:6px 0;color:#5f6368}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(132px,1fr));gap:8px}
button{min-height:42px;border:1px solid #c9ced6;background:#fff;border-radius:8px;font-size:15px;cursor:pointer;touch-action:manipulation;-webkit-tap-highlight-color:transparent;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}
button:active{transform:translateY(1px)}
.primary{background:#174ea6;color:#fff;border-color:#174ea6}
.danger{background:#b3261e;color:#fff;border-color:#b3261e}
.toolbar{display:flex;justify-content:flex-end;margin:8px 0 12px}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px;align-items:center}
label{display:flex;gap:8px;align-items:center;margin:8px 0}
input[type=number],input[type=text]{width:100%;box-sizing:border-box;padding:9px;border:1px solid #c9ced6;border-radius:6px;font-size:15px}
pre{white-space:pre-wrap;word-break:break-word;background:#111;color:#d9fdd3;border-radius:8px;padding:12px;min-height:160px}
pre,input{-webkit-touch-callout:default;-webkit-user-select:text;user-select:text}
.radarCard{background:#fff;border:1px solid #d5d9df;border-radius:8px;padding:12px}
.radarHead{display:flex;gap:12px;align-items:center;margin-bottom:10px}
.radarDot{width:46px;height:46px;border-radius:50%;background:#9aa0a6;box-shadow:0 0 0 6px #eef0f2}
.radarDot.seen{background:#188038;box-shadow:0 0 0 6px #dff3e7}
.radarDot.waiting{background:#f29900;box-shadow:0 0 0 6px #fff3d6}
.radarState{font-size:19px;font-weight:700}
.kv{display:grid;grid-template-columns:1fr 1fr;gap:6px 10px;margin-top:10px}
.kv div{background:#f6f7f8;border-radius:6px;padding:8px}
.small{font-size:13px;color:#5f6368}
</style>
</head>
<body>
<main>
<h1 data-i18n="title">TongDou V9 Board Test</h1>
<div class="toolbar"><button id="langToggle" onclick="toggleLanguage()">中文</button></div>
<p>AP: TongDou-BoardTest · Page: 192.168.4.1/motor</p>

<h2 data-i18n="status">Status</h2>
<div class="grid">
<button onclick="refreshStatus()" data-i18n="refresh">Refresh</button>
<button onclick="copyLog()" data-i18n="copyLog">Copy log</button>
</div>
<pre id="status">loading...</pre>

<h2 data-i18n="quickChecks">Quick Checks</h2>
<div class="grid">
<button onclick="diag('selftest')" data-i18n="selfTest">Self Test</button>
<button onclick="diag('battery')" data-i18n="battery">Battery</button>
<button onclick="diag('mic')" data-i18n="mic">Mic</button>
<button onclick="diag('speaker')" data-i18n="speaker">Speaker</button>
<button onclick="diag('i2c scan')" data-i18n="i2cScan">I2C Scan</button>
<button onclick="diag('imu')" data-i18n="imu">IMU</button>
<button onclick="diag('imu raw test')" data-i18n="imuRawTest">IMU Raw Test</button>
<button onclick="diag('radar')" data-i18n="radar">Radar</button>
</div>
<pre id="last" data-idle="1">Tap a test button to run it.</pre>

<h2 data-i18n="radarPanel">Radar Recognition</h2>
<div class="radarCard">
<div class="radarHead">
<div id="radarDot" class="radarDot"></div>
<div>
<div id="radarState" class="radarState" data-i18n="radarNotTested">Not tested</div>
<p id="radarHint" class="small" data-i18n="radarHint">Put your hand in front of the radar, then start live test.</p>
</div>
</div>
<div class="grid">
<button onclick="readRadarOnce()" data-i18n="radarOnce">Read once</button>
<button class="primary" onclick="startRadarLive()" data-i18n="radarStart">Start live</button>
<button onclick="stopRadarLive()" data-i18n="radarStop">Stop live</button>
<button onclick="radarGuidedStart()" data-i18n="radarGuided">Guided test</button>
<button onclick="radarDeskMode()" data-i18n="radarDesk">Desk mode</button>
</div>
</div>

<h2>LED</h2>
<div class="grid">
<button onclick="diag('led red')" data-i18n="red">Red</button>
<button onclick="diag('led green')" data-i18n="green">Green</button>
<button onclick="diag('led blue')" data-i18n="blue">Blue</button>
<button onclick="diag('led off')" data-i18n="off">Off</button>
</div>

<h2 data-i18n="motorTitle">Motor Direction And Power</h2>
<div class="row">
<label><span data-i18n="leftPwm">Left PWM</span> <input id="leftPwm" type="number" min="0" max="255"></label>
<label><span data-i18n="rightPwm">Right PWM</span> <input id="rightPwm" type="number" min="0" max="255"></label>
</div>
<div class="grid">
<button class="danger" onclick="diag('motor stop')" data-i18n="stop">Stop</button>
<button onclick="diag('motor forward')" data-i18n="pulseForward">Pulse forward</button>
<button onclick="diag('motor reverse')" data-i18n="pulseReverse">Pulse reverse</button>
<button onclick="manualMotor('forward')" data-i18n="manualForward">Manual forward</button>
<button onclick="manualMotor('reverse')" data-i18n="manualReverse">Manual reverse</button>
</div>
<p class="small" data-i18n="motorNote">Manual motor commands time out automatically. Use Stop before lifting the board.</p>

<h2 data-i18n="serialCommand">Serial Style Command</h2>
<div class="row">
<input id="cmd" type="text" placeholder="example: audio volume 60">
<button onclick="diag(document.getElementById('cmd').value)" data-i18n="run">Run</button>
</div>

<h2 data-i18n="log">Log</h2>
<pre id="log"></pre>
</main>
<script>
const $=id=>document.getElementById(id);
let currentLanguage='en';
let radarTimer=null;
let radarBusy=false;
let radarGuidedTimer=null;
const text={
  en:{
    title:'TongDou V9 Board Test',
    subtitle:'AP: TongDou-BoardTest · Page: 192.168.4.1/motor',
    langToggle:'中文',
    status:'Status',
    refresh:'Refresh',
    copyLog:'Copy log',
    quickChecks:'Quick Checks',
    selfTest:'Self Test',
    battery:'Battery',
    mic:'Mic',
    speaker:'Speaker',
    i2cScan:'I2C Scan',
    imu:'IMU',
    imuRawTest:'IMU Raw Test',
    radar:'Radar',
    lastIdle:'Tap a test button to run it.',
    running:'Running',
    requestFailed:'Request failed',
    radarPanel:'Radar Recognition',
    radarNotTested:'Not tested',
    radarHint:'Put your hand in front of the radar, then start live test.',
    radarOnce:'Read once',
    radarStart:'Start live',
    radarStop:'Stop live',
    radarGuided:'Guided test',
    radarDesk:'Desk mode',
    radarSeen:'Target detected',
    radarClear:'No target',
    radarNoFrame:'No serial frame',
    yes:'yes',
    no:'no',
    cm:'cm',
    red:'Red',
    green:'Green',
    blue:'Blue',
    off:'Off',
    motorTitle:'Motor Direction And Power',
    leftPwm:'Left PWM',
    rightPwm:'Right PWM',
    stop:'Stop',
    pulseForward:'Pulse forward',
    pulseReverse:'Pulse reverse',
    manualForward:'Manual forward',
    manualReverse:'Manual reverse',
    motorNote:'Manual motor commands time out automatically. Use Stop before lifting the board.',
    serialCommand:'Serial Style Command',
    run:'Run',
    log:'Log',
    cmdPlaceholder:'example: audio volume 60'
  },
  zh:{
    title:'铜豆 V9 板测',
    subtitle:'热点：TongDou-BoardTest · 页面：192.168.4.1/motor',
    langToggle:'English',
    status:'状态',
    refresh:'刷新',
    copyLog:'复制日志',
    quickChecks:'快速检查',
    selfTest:'一键自检',
    battery:'电池',
    mic:'麦克风',
    speaker:'喇叭',
    i2cScan:'I2C 扫描',
    imu:'陀螺仪',
    imuRawTest:'陀螺仪原始测试',
    radar:'雷达',
    lastIdle:'点击测试按钮后，这里会显示结果。',
    running:'正在执行',
    requestFailed:'请求失败',
    radarPanel:'雷达识别',
    radarNotTested:'未测试',
    radarHint:'把手放到雷达前方，然后开始实时测试。',
    radarOnce:'读取一次',
    radarStart:'开始实时',
    radarStop:'停止实时',
    radarDesk:'桌面模式',
    radarSeen:'识别到目标',
    radarClear:'没有目标',
    radarNoFrame:'没有串口数据',
    yes:'有',
    no:'无',
    cm:'厘米',
    red:'红灯',
    green:'绿灯',
    blue:'蓝灯',
    off:'关闭',
    motorTitle:'电机方向和力度',
    leftPwm:'左轮 PWM',
    rightPwm:'右轮 PWM',
    stop:'停止',
    pulseForward:'短促前进',
    pulseReverse:'短促后退',
    manualForward:'手动前进',
    manualReverse:'手动后退',
    motorNote:'手动电机会自动超时。拿起板子前先点停止。',
    serialCommand:'串口式命令',
    run:'执行',
    log:'日志',
    cmdPlaceholder:'示例：audio volume 60'
  }
};
function applyLanguage(language){
  currentLanguage=language;
  const dict=text[language];
  document.documentElement.lang=language;
  document.querySelectorAll('[data-i18n]').forEach(el=>{
    const key=el.dataset.i18n;
    if(dict[key])el.textContent=dict[key];
  });
  const subtitle=document.querySelector('main>p');
  if(subtitle)subtitle.textContent=dict.subtitle;
  $('langToggle').textContent=dict.langToggle;
  $('cmd').placeholder=dict.cmdPlaceholder;
  if($('last').dataset.idle==='1')$('last').textContent=dict.lastIdle;
}
function toggleLanguage(){
  applyLanguage(currentLanguage==='en'?'zh':'en');
}
document.addEventListener('contextmenu',event=>{
  if(event.target.closest('button'))event.preventDefault();
});
document.addEventListener('selectstart',event=>{
  if(event.target.closest('button'))event.preventDefault();
});
function append(text){$('log').textContent=(text||'')+'\n\n'+$('log').textContent}
async function post(path, body){
  const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  return await r.text();
}
function updateRadarPanel(data){
  const dict=text[currentLanguage];
  const seen=!!data.hasTarget;
  const frame=!!data.received;
  if(!frame){
    $('radarDot').className='radarDot';
    $('radarState').textContent=dict.radarNoFrame;
    $('radarHint').textContent=dict.radarNoFrame;
    return;
  }
  $('radarDot').className='radarDot '+(seen?'seen':'waiting');
  $('radarState').textContent=seen?dict.radarSeen:dict.radarClear;
  $('radarHint').textContent=seen?dict.radarSeen:dict.radarClear;
}
async function readRadarOnce(){
  if(radarBusy)return;
  radarBusy=true;
  const dict=text[currentLanguage];
  $('radarHint').textContent=dict.running+': radar ...';
  const controller=new AbortController();
  const timeout=setTimeout(()=>controller.abort(),1200);
  try{
    const r=await fetch('/api/radar?t='+Date.now(),{cache:'no-store',signal:controller.signal});
    const data=await r.json();
    updateRadarPanel(data);
  }catch(error){
    $('radarDot').className='radarDot';
    $('radarState').textContent=dict.requestFailed;
    $('radarHint').textContent=dict.requestFailed+': '+error;
  }finally{
    clearTimeout(timeout);
    radarBusy=false;
  }
}
function startRadarLive(){
  stopRadarLive();
  readRadarOnce();
  radarTimer=setInterval(readRadarOnce,400);
}
function stopRadarLive(){
  if(radarTimer){
    clearInterval(radarTimer);
    radarTimer=null;
  }
}
function stopRadarGuidedPoll(){
  if(radarGuidedTimer){
    clearInterval(radarGuidedTimer);
    radarGuidedTimer=null;
  }
}
async function radarGuidedStatus(){
  try{
    const result=await post('/api/diagnostic','command=radar%20guided%20status');
    $('last').textContent='$ radar guided status\n'+result;
    append('$ radar guided status\n'+result);
    if(result.includes('done=1'))stopRadarGuidedPoll();
  }catch(error){
    stopRadarGuidedPoll();
    const message=text[currentLanguage].requestFailed+': '+error;
    $('last').textContent='$ radar guided status\n'+message;
    append('$ radar guided status\n'+message);
  }
}
async function radarGuidedStart(){
  stopRadarLive();
  stopRadarGuidedPoll();
  $('last').dataset.idle='0';
  append('$ radar guided test\n'+text[currentLanguage].running+' ...');
  try{
    const result=await post('/api/diagnostic','command=radar%20guided%20test');
    $('last').textContent='$ radar guided test\n'+result;
    append('$ radar guided test\n'+result);
    radarGuidedTimer=setInterval(radarGuidedStatus,1000);
  }catch(error){
    const message=text[currentLanguage].requestFailed+': '+error;
    $('last').textContent='$ radar guided test\n'+message;
    append('$ radar guided test\n'+message);
  }
}
async function radarDeskMode(){
  stopRadarLive();
  stopRadarGuidedPoll();
  await diag('radar desk');
  setTimeout(()=>diag('radar resolution'),3200);
  setTimeout(()=>diag('radar config'),3800);
}
async function diag(command){
  if(!command)return;
  stopRadarLive();
  stopRadarGuidedPoll();
  $('last').dataset.idle='0';
  $('last').textContent=text[currentLanguage].running+': '+command+' ...';
  append('$ '+command+'\n'+text[currentLanguage].running+' ...');
  try{
    const result=await post('/api/diagnostic','command='+encodeURIComponent(command));
    $('last').textContent='$ '+command+'\n'+result;
    append('$ '+command+'\n'+result);
    refreshStatus();
  }catch(error){
    const message=text[currentLanguage].requestFailed+': '+error;
    $('last').textContent='$ '+command+'\n'+message;
    append('$ '+command+'\n'+message);
  }
}
async function refreshStatus(){
  const r=await fetch('/api/status');
  const j=await r.json();
  $('status').textContent=JSON.stringify(j,null,2);
  if(!$('leftPwm').value)$('leftPwm').value=j.motorLeftPwm;
  if(!$('rightPwm').value)$('rightPwm').value=j.motorRightPwm;
}
function manualMotor(direction){
  diag('motor manual '+direction+' '+$('leftPwm').value+' '+$('rightPwm').value);
}
async function copyLog(){
  const text='STATUS\n'+$('status').textContent+'\n\nLOG\n'+$('log').textContent;
  await navigator.clipboard.writeText(text);
}
window.addEventListener('beforeunload',()=>navigator.sendBeacon('/api/diagnostic','command=motor%20stop'));
applyLanguage('en');
refreshStatus();
</script>
</body>
</html>
)HTML";

  sendText(200, "text/html; charset=utf-8", FPSTR(kPage));
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

void WebConfigServer::handleRadarStatus() {
  const RadarSnapshot pinSnapshot = hardwareSelfTest_.radarSnapshot();
  const RadarTargetSnapshot target = hardwareSelfTest_.radarTargetSnapshot();

  Serial.print(F("radar live seq="));
  Serial.print(target.sequence);
  Serial.print(F(" occupied="));
  Serial.print(pinSnapshot.occupied ? 1 : 0);
  Serial.print(F(" rx="));
  Serial.print(pinSnapshot.rxLevelHigh ? 1 : 0);
  Serial.print(F(" received="));
  Serial.print(target.received ? 1 : 0);
  Serial.print(F(" target="));
  Serial.print(target.hasTarget ? 1 : 0);
  Serial.print(F(" state_target="));
  Serial.print(target.stateTarget ? 1 : 0);
  Serial.print(F(" energy_target="));
  Serial.print(target.energyTarget ? 1 : 0);
  Serial.print(F(" confidence="));
  Serial.print(target.targetConfidence);
  Serial.print(F(" state="));
  Serial.print(target.targetState);
  Serial.print(F(" moving_cm="));
  Serial.print(target.movingDistanceCm);
  Serial.print(F(" moving_energy="));
  Serial.print(target.movingEnergy);
  Serial.print(F(" static_cm="));
  Serial.print(target.staticDistanceCm);
  Serial.print(F(" static_energy="));
  Serial.print(target.staticEnergy);
  Serial.print(F(" age_ms="));
  Serial.print(target.frameAgeMs);
  Serial.print(F(" valid_frames="));
  Serial.print(target.validFrameCount);
  Serial.print(F(" invalid_frames="));
  Serial.println(target.invalidFrameCount);

  String body = "{";
  body += "\"sequence\":";
  body += target.sequence;
  body += ",\"occupied\":";
  body += boolJson(pinSnapshot.occupied);
  body += ",\"rxLevelHigh\":";
  body += boolJson(pinSnapshot.rxLevelHigh);
  body += ",\"received\":";
  body += boolJson(target.received);
  body += ",\"hasTarget\":";
  body += boolJson(target.hasTarget);
  body += ",\"stateTarget\":";
  body += boolJson(target.stateTarget);
  body += ",\"energyTarget\":";
  body += boolJson(target.energyTarget);
  body += ",\"targetConfidence\":";
  body += target.targetConfidence;
  body += ",\"state\":";
  body += target.targetState;
  body += ",\"targetDistanceCm\":";
  body += target.targetDistanceCm;
  body += ",\"movingDistanceCm\":";
  body += target.movingDistanceCm;
  body += ",\"movingEnergy\":";
  body += target.movingEnergy;
  body += ",\"staticDistanceCm\":";
  body += target.staticDistanceCm;
  body += ",\"staticEnergy\":";
  body += target.staticEnergy;
  body += ",\"frameAgeMs\":";
  body += target.frameAgeMs;
  body += ",\"validFrameCount\":";
  body += target.validFrameCount;
  body += ",\"invalidFrameCount\":";
  body += target.invalidFrameCount;
  body += "}";
  sendText(200, "application/json", body);
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
