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
button:disabled{opacity:.55;cursor:default;transform:none}
.primary{background:#174ea6;color:#fff;border-color:#174ea6}
.danger{background:#b3261e;color:#fff;border-color:#b3261e}
.toolbar{display:flex;justify-content:space-between;align-items:center;gap:8px;margin:8px 0 12px}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px;align-items:center}
label{display:flex;gap:8px;align-items:center;margin:8px 0}
input[type=number],input[type=text]{width:100%;box-sizing:border-box;padding:9px;border:1px solid #c9ced6;border-radius:6px;font-size:15px}
input[type=range]{width:100%;touch-action:pan-x}
pre{white-space:pre-wrap;word-break:break-word;background:#111;color:#d9fdd3;border-radius:8px;padding:12px;min-height:110px;max-height:360px;overflow:auto}
pre,input{-webkit-touch-callout:default;-webkit-user-select:text;user-select:text}
.statusGrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
.statusItem{display:flex;justify-content:space-between;align-items:center;gap:8px;background:#fff;border:1px solid #d5d9df;border-radius:8px;padding:10px 12px}
.statusValue{font-weight:700;color:#b3261e}
.statusValue.ok{color:#188038}
.radarCard{background:#fff;border:1px solid #d5d9df;border-radius:8px;padding:12px}
.radarHead{display:flex;gap:12px;align-items:center;margin-bottom:10px}
.radarDot{width:46px;height:46px;border-radius:50%;background:#9aa0a6;box-shadow:0 0 0 6px #eef0f2}
.radarDot.seen{background:#188038;box-shadow:0 0 0 6px #dff3e7}
.radarDot.waiting{background:#f29900;box-shadow:0 0 0 6px #fff3d6}
.radarState{font-size:19px;font-weight:700}
.kv{display:grid;grid-template-columns:1fr 1fr;gap:6px 10px;margin-top:10px}
.kv div{background:#f6f7f8;border-radius:6px;padding:8px}
.controlBand{background:#fff;border:1px solid #d5d9df;border-radius:8px;padding:12px}
.rangeRow{display:grid;grid-template-columns:1fr 48px 112px;gap:10px;align-items:center}
.rangeValue{text-align:center;font-weight:700}
.sectionActions{display:flex;justify-content:flex-end;gap:8px;margin-bottom:8px}
.sectionActions button{min-height:36px;font-size:13px}
details{margin-top:22px}
summary{font-size:17px;font-weight:700;cursor:pointer;padding:8px 0}
.small{font-size:13px;color:#5f6368}
@media(max-width:520px){
  body{padding:12px}
  .row,.statusGrid{grid-template-columns:1fr}
  .rangeRow{grid-template-columns:1fr 44px}
  .rangeRow button{grid-column:1/-1}
}
</style>
</head>
<body>
<main>
<h1 data-i18n="title">TongDou V9 Board Test</h1>
<div class="toolbar">
<p id="subtitle">AP: TongDou-BoardTest · Page: 192.168.4.1/motor</p>
<button id="langToggle" onclick="toggleLanguage()">中文</button>
</div>

<h2 data-i18n="status">Status</h2>
<div class="statusGrid">
<div class="statusItem"><span data-i18n="display">Display</span><span id="displayStatus" class="statusValue">--</span></div>
<div class="statusItem"><span data-i18n="led">LED</span><span id="ledStatus" class="statusValue">--</span></div>
<div class="statusItem"><span data-i18n="mic">Microphone</span><span id="micStatus" class="statusValue">--</span></div>
<div class="statusItem"><span data-i18n="speaker">Speaker</span><span id="speakerStatus" class="statusValue">--</span></div>
<div class="statusItem"><span data-i18n="power">Power</span><span id="powerStatus" class="statusValue">--</span></div>
<div class="statusItem"><span data-i18n="system">System</span><span id="systemStatus" class="statusValue">--</span></div>
</div>

<h2 data-i18n="quickChecks">Quick Checks</h2>
<div class="grid">
<button onclick="diag('selftest')" data-i18n="selfTest">Self Test</button>
<button onclick="diag('battery')" data-i18n="battery">Battery</button>
<button onclick="diag('mic')" data-i18n="mic">Mic</button>
<button onclick="diag('speaker')" data-i18n="speaker">Speaker</button>
<button onclick="diag('i2c scan')" data-i18n="i2cScan">I2C Scan</button>
<button onclick="diag('imu')" data-i18n="imu">IMU</button>
</div>
<pre id="last" data-idle="1">Tap a test button to run it.</pre>

<h2 data-i18n="radarPanel">Radar Recognition</h2>
<div class="radarCard">
<div class="radarHead">
<div id="radarDot" class="radarDot"></div>
<div>
<div id="radarState" class="radarState" data-i18n="radarNotTested">Not tested</div>
<p id="radarHint" class="small" data-i18n="radarReady">Ready</p>
</div>
</div>
<div class="grid">
<button class="primary" onclick="startRadarLive()" data-i18n="radarStart">Start live</button>
<button onclick="stopRadarLive()" data-i18n="radarStop">Stop live</button>
<button id="radarCalibrate" onclick="startRadarCalibration()" data-i18n="radarCalibrate">Calibrate empty</button>
</div>
<div class="kv">
<div><span class="small" data-i18n="radarMotion">Motion target</span><br><strong id="radarMotion">-</strong></div>
<div><span class="small" data-i18n="radarDistance">Nearest distance</span><br><strong id="radarDistance">-</strong></div>
<div><span class="small" data-i18n="radarValidFrames">Valid frames</span><br><strong id="radarValidFrames">0</strong></div>
<div><span class="small" data-i18n="radarBadFrames">Bad frames</span><br><strong id="radarBadFrames">0</strong></div>
</div>
</div>

<h2 data-i18n="soundLight">Sound And Light</h2>
<div class="controlBand">
<label for="volume" data-i18n="volume">Speaker volume</label>
<div class="rangeRow">
<input id="volume" type="range" min="0" max="100" value="60" oninput="showVolume()">
<span id="volumeValue" class="rangeValue">60%</span>
<button onclick="saveVolume()" data-i18n="applyVolume">Apply volume</button>
</div>
</div>
<div class="grid" style="margin-top:8px">
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

<details open>
<summary data-i18n="log">Log</summary>
<div class="sectionActions">
<button onclick="copyLog()" data-i18n="copyLog">Copy log</button>
<button onclick="clearLog()" data-i18n="clearLog">Clear log</button>
</div>
<pre id="log"></pre>
</details>
</main>
<script>
const $=id=>document.getElementById(id);
let currentLanguage='en';
let radarTimer=null;
let radarBusy=false;
let radarCalibrationTimer=null;
let calibrationInProgress=false;
let lastStatus=null;
let lastRadarData=null;
let radarNoticeKey=null;
const text={
  en:{
    title:'TongDou V9 Board Test',
    subtitle:'AP: TongDou-BoardTest · Page: 192.168.4.1/motor',
    langToggle:'中文',
    status:'Status',
    display:'Display',
    led:'LED',
    power:'Power',
    system:'System',
    ready:'Ready',
    check:'Check',
    usbPower:'USB power',
    batteryPower:'Battery power',
    charging:'Charging',
    standby:'Standby',
    healthy:'Healthy',
    degraded:'Degraded',
    copyLog:'Copy log',
    clearLog:'Clear log',
    quickChecks:'Quick Checks',
    selfTest:'Self Test',
    battery:'Battery',
    mic:'Mic',
    speaker:'Speaker',
    i2cScan:'I2C Scan',
    imu:'IMU',
    lastIdle:'Tap a test button to run it.',
    running:'Running',
    requestFailed:'Request failed',
    radarPanel:'Radar Recognition',
    radarNotTested:'Not tested',
    radarReady:'Ready',
    radarStart:'Start live',
    radarStop:'Stop live',
    radarCalibrate:'Calibrate empty',
    radarCalibrating:'Calibrating empty scene',
    radarCalComplete:'Calibration complete',
    radarCalFailed:'Calibration failed',
    radarSeen:'Target detected',
    radarClear:'No target',
    radarNoFrame:'No serial frame',
    radarMotion:'Motion target',
    radarDistance:'Nearest distance',
    radarValidFrames:'Valid frames',
    radarBadFrames:'Bad frames',
    yes:'yes',
    no:'no',
    cm:'cm',
    soundLight:'Sound And Light',
    volume:'Speaker volume',
    applyVolume:'Apply volume',
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
    log:'Log'
  },
  zh:{
    title:'铜豆 V9 板测',
    subtitle:'热点：TongDou-BoardTest · 页面：192.168.4.1/motor',
    langToggle:'English',
    status:'状态',
    display:'屏幕',
    led:'灯珠',
    power:'供电',
    system:'系统',
    ready:'正常',
    check:'检查',
    usbPower:'USB 供电',
    batteryPower:'电池供电',
    charging:'充电中',
    standby:'已充满',
    healthy:'正常',
    degraded:'降级',
    copyLog:'复制日志',
    clearLog:'清空日志',
    quickChecks:'快速检查',
    selfTest:'一键自检',
    battery:'电池',
    mic:'麦克风',
    speaker:'喇叭',
    i2cScan:'I2C 扫描',
    imu:'陀螺仪',
    lastIdle:'点击测试按钮后，这里会显示结果。',
    running:'正在执行',
    requestFailed:'请求失败',
    radarPanel:'雷达识别',
    radarNotTested:'未测试',
    radarReady:'就绪',
    radarStart:'开始实时',
    radarStop:'停止实时',
    radarCalibrate:'空场校准',
    radarCalibrating:'正在校准空场',
    radarCalComplete:'校准完成',
    radarCalFailed:'校准失败',
    radarSeen:'识别到目标',
    radarClear:'没有目标',
    radarNoFrame:'没有串口数据',
    radarMotion:'运动目标',
    radarDistance:'最近距离',
    radarValidFrames:'有效帧',
    radarBadFrames:'坏帧',
    yes:'有',
    no:'无',
    cm:'厘米',
    soundLight:'声音与灯光',
    volume:'喇叭音量',
    applyVolume:'应用音量',
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
    log:'日志'
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
  $('subtitle').textContent=dict.subtitle;
  $('langToggle').textContent=dict.langToggle;
  if($('last').dataset.idle==='1')$('last').textContent=dict.lastIdle;
  if(lastStatus)renderStatus(lastStatus);
  if(calibrationInProgress){
    $('radarState').textContent=dict.radarCalibrating;
    $('radarHint').textContent=dict.radarCalibrating;
  }else if(radarNoticeKey){
    $('radarState').textContent=dict[radarNoticeKey];
    $('radarHint').textContent=dict[radarNoticeKey];
  }else if(lastRadarData){
    updateRadarPanel(lastRadarData);
  }
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
  radarNoticeKey=null;
  lastRadarData=data;
  const dict=text[currentLanguage];
  const seen=!!data.hasTarget;
  const moving=!!data.movingTarget;
  const frame=!!data.received;
  const badFrames=data.badFrameCount||data.invalidFrameCount||0;
  if(!frame){
    $('radarDot').className='radarDot';
    $('radarState').textContent=dict.radarNoFrame;
    $('radarHint').textContent=dict.radarNoFrame;
    $('radarMotion').textContent='-';
    $('radarDistance').textContent='-';
    $('radarValidFrames').textContent=data.validFrameCount||0;
    $('radarBadFrames').textContent=badFrames;
    return;
  }
  const distance=data.nearestDistanceCm||data.targetDistanceCm||0;
  $('radarDot').className='radarDot '+(seen?'seen':'waiting');
  $('radarState').textContent=seen?dict.radarSeen:dict.radarClear;
  $('radarHint').textContent=(seen?dict.radarSeen:dict.radarClear)+' · '+dict.radarMotion+': '+(moving?dict.yes:dict.no)+' · '+dict.radarBadFrames+': '+badFrames;
  $('radarMotion').textContent=moving?dict.yes:dict.no;
  $('radarDistance').textContent=distance>0?distance+' '+dict.cm:'-';
  $('radarValidFrames').textContent=data.validFrameCount||0;
  $('radarBadFrames').textContent=badFrames;
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
  if(calibrationInProgress)return;
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
function stopRadarCalibrationPoll(){
  if(radarCalibrationTimer){
    clearInterval(radarCalibrationTimer);
    radarCalibrationTimer=null;
  }
}
function finishRadarCalibration(success,result){
  stopRadarCalibrationPoll();
  calibrationInProgress=false;
  $('radarCalibrate').disabled=false;
  const dict=text[currentLanguage];
  radarNoticeKey=success?'radarCalComplete':'radarCalFailed';
  $('radarDot').className='radarDot '+(success?'seen':'');
  $('radarState').textContent=dict[radarNoticeKey];
  $('radarHint').textContent=dict[radarNoticeKey];
  append('$ radar calibrate status\n'+result);
}
async function pollRadarCalibration(){
  try{
    const result=await post('/api/diagnostic','command=radar%20calibrate%20status');
    if(!result.includes('received=1')||!result.includes('success=1')){
      finishRadarCalibration(false,result);
      return;
    }
    if(result.includes('running=0'))finishRadarCalibration(true,result);
  }catch(error){
    finishRadarCalibration(false,text[currentLanguage].requestFailed+': '+error);
  }
}
async function startRadarCalibration(){
  stopRadarLive();
  stopRadarCalibrationPoll();
  calibrationInProgress=true;
  lastRadarData=null;
  radarNoticeKey=null;
  $('radarCalibrate').disabled=true;
  $('radarDot').className='radarDot waiting';
  $('radarState').textContent=text[currentLanguage].radarCalibrating;
  $('radarHint').textContent=text[currentLanguage].radarCalibrating;
  $('last').dataset.idle='0';
  $('last').textContent=text[currentLanguage].radarCalibrating+' ...';
  try{
    const result=await post('/api/diagnostic','command=radar%20calibrate');
    $('last').textContent='$ radar calibrate\n'+result;
    append('$ radar calibrate\n'+result);
    if(!result.includes('received=1')||!result.includes('success=1')){
      finishRadarCalibration(false,result);
      return;
    }
    setTimeout(()=>{
      if(!calibrationInProgress)return;
      pollRadarCalibration();
      radarCalibrationTimer=setInterval(pollRadarCalibration,5000);
    },12000);
  }catch(error){
    finishRadarCalibration(false,text[currentLanguage].requestFailed+': '+error);
  }
}
async function diag(command){
  if(!command)return;
  stopRadarLive();
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
function setStatus(id,ok,label){
  const element=$(id);
  element.textContent=label;
  element.className='statusValue'+(ok?' ok':'');
}
function renderStatus(data){
  const dict=text[currentLanguage];
  setStatus('displayStatus',data.faceReady,data.faceReady?dict.ready:dict.check);
  setStatus('ledStatus',data.ledReady,data.ledReady?dict.ready:dict.check);
  setStatus('micStatus',data.audioInputReady,data.audioInputReady?dict.ready:dict.check);
  setStatus('speakerStatus',data.audioOutputReady,data.audioOutputReady?dict.ready:dict.check);
  let power=dict.batteryPower;
  if(data.usbPresent)power=data.charging?dict.charging:(data.standby?dict.standby:dict.usbPower);
  setStatus('powerStatus',true,power);
  setStatus('systemStatus',!data.degraded,data.degraded?dict.degraded:dict.healthy);
  if(document.activeElement!==$('volume')){
    $('volume').value=data.audioVolumePercent;
    showVolume();
  }
}
async function refreshStatus(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    lastStatus=await r.json();
    renderStatus(lastStatus);
    if(!$('leftPwm').value)$('leftPwm').value=lastStatus.motorLeftPwm;
    if(!$('rightPwm').value)$('rightPwm').value=lastStatus.motorRightPwm;
  }catch(error){
    append(text[currentLanguage].requestFailed+': '+error);
  }
}
function showVolume(){
  $('volumeValue').textContent=$('volume').value+'%';
}
async function saveVolume(){
  await diag('audio volume '+$('volume').value);
}
function manualMotor(direction){
  diag('motor manual '+direction+' '+$('leftPwm').value+' '+$('rightPwm').value);
}
async function copyLog(){
  const value='STATUS\n'+JSON.stringify(lastStatus||{},null,2)+'\n\nLOG\n'+$('log').textContent;
  try{
    await navigator.clipboard.writeText(value);
  }catch(error){
    const area=document.createElement('textarea');
    area.value=value;
    document.body.appendChild(area);
    area.select();
    document.execCommand('copy');
    area.remove();
  }
}
function clearLog(){$('log').textContent=''}
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
  body += ",\"movingTarget\":";
  body += boolJson((target.targetState & 0x01) != 0);
  body += ",\"staticTarget\":";
  body += boolJson((target.targetState & 0x02) != 0);
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
  body += ",\"nearestDistanceCm\":";
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
  body += ",\"badFrameCount\":";
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
