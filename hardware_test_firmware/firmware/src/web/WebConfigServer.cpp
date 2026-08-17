#include "web/WebConfigServer.h"

#include <WiFi.h>
#include <math.h>
#include <string.h>

namespace tongdou {
namespace {

constexpr const char* kPortalSsid = "TongDou-BoardTest";
constexpr uint8_t kPortalChannel = 6;
constexpr uint8_t kPortalMaxConnections = 4;
constexpr unsigned long kApHealthLogIntervalMs = 15000;
constexpr size_t kAudioWavHeaderBytes = 44;
constexpr size_t kAudioExportChunkSamples = 128;
constexpr int32_t kAudioHighPassAlphaQ15 = 31295;  // about 120 Hz at 16 kHz.
constexpr float kAudioExportTargetRms = 2000.0F;
constexpr float kAudioExportMaxGain = 40.0F;
const IPAddress kPortalIp(192, 168, 4, 1);
const IPAddress kPortalGateway(192, 168, 4, 1);
const IPAddress kPortalSubnet(255, 255, 255, 0);
const IPAddress kPortalDhcpStart(192, 168, 4, 2);

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

String jsonString(const char* value) {
  String escaped = "\"";
  if (value != nullptr) {
    while (*value != '\0') {
      if (*value == '"' || *value == '\\') {
        escaped += '\\';
      }
      escaped += *value;
      ++value;
    }
  }
  escaped += "\"";
  return escaped;
}

void writeLe16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value & 0xFF);
  output[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void writeLe32(uint8_t* output, uint32_t value) {
  output[0] = static_cast<uint8_t>(value & 0xFF);
  output[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  output[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  output[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void buildWavHeader(uint8_t* header, uint32_t sampleRate, uint32_t sampleCount) {
  const uint32_t dataBytes = sampleCount * sizeof(int16_t);
  memcpy(header + 0, "RIFF", 4);
  writeLe32(header + 4, 36U + dataBytes);
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);
  writeLe32(header + 16, 16);
  writeLe16(header + 20, 1);
  writeLe16(header + 22, 1);
  writeLe32(header + 24, sampleRate);
  writeLe32(header + 28, sampleRate * sizeof(int16_t));
  writeLe16(header + 32, sizeof(int16_t));
  writeLe16(header + 34, 16);
  memcpy(header + 36, "data", 4);
  writeLe32(header + 40, dataBytes);
}

int16_t clampPcm16(int32_t value) {
  if (value > 32767) {
    return 32767;
  }
  if (value < -32768) {
    return -32768;
  }
  return static_cast<int16_t>(value);
}

int32_t highPassPcm16(int32_t sample, int32_t& previousInput,
                      int32_t& previousOutput) {
  const int32_t output =
      (kAudioHighPassAlphaQ15 * (previousOutput + sample - previousInput)) >> 15;
  previousInput = sample;
  previousOutput = output;
  return output;
}

}  // namespace

WebConfigServer::WebConfigServer(HardwareSelfTestService& hardwareSelfTest)
    : hardwareSelfTest_(hardwareSelfTest) {}

void WebConfigServer::begin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  const bool configReady =
      WiFi.softAPConfig(kPortalIp, kPortalGateway, kPortalSubnet,
                        kPortalDhcpStart);
  const bool apReady =
      WiFi.softAP(kPortalSsid, nullptr, kPortalChannel, 0,
                  kPortalMaxConnections);

  dnsStarted_ = false;

  setupRoutes();
  server_.begin();
  serverStarted_ = true;

  Serial.print(F("board test ap config="));
  Serial.print(configReady ? 1 : 0);
  Serial.print(F(" start="));
  Serial.print(apReady ? 1 : 0);
  Serial.print(F(" ssid="));
  Serial.print(kPortalSsid);
  Serial.print(F(" channel="));
  Serial.print(kPortalChannel);
  Serial.print(F(" max_clients="));
  Serial.print(kPortalMaxConnections);
  Serial.print(F(" dns="));
  Serial.print(dnsStarted_ ? 1 : 0);
  Serial.print(F(" ip="));
  Serial.print(WiFi.softAPIP());
  Serial.print(F(" dhcp_start="));
  Serial.println(kPortalDhcpStart);
}

void WebConfigServer::update() {
  if (dnsStarted_) {
    dnsServer_.processNextRequest();
  }
  if (serverStarted_) {
    server_.handleClient();
  }

  const uint8_t stationCount = WiFi.softAPgetStationNum();
  if (stationCount != lastStationCount_) {
    lastStationCount_ = stationCount;
    Serial.print(F("board test ap clients="));
    Serial.println(lastStationCount_);
  }
  if (millis() - lastApHealthLogMs_ >= kApHealthLogIntervalMs) {
    lastApHealthLogMs_ = millis();
    Serial.print(F("board test ap health ip="));
    Serial.print(WiFi.softAPIP());
    Serial.print(F(" clients="));
    Serial.println(stationCount);
  }
}

void WebConfigServer::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/motor", HTTP_GET, [this]() { handleBoardTestPage(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/radar", HTTP_GET, [this]() { handleRadarStatus(); });
  server_.on("/api/diagnostic", HTTP_POST, [this]() { handleDiagnosticCommand(); });
  server_.on("/api/audio/recording.wav", HTTP_GET,
             [this]() { handleAudioRecordingDownload(); });
  server_.on("/api/motor/config", HTTP_GET, [this]() { handleMotorConfigGet(); });
  server_.on("/api/motor/config", HTTP_POST, [this]() { handleMotorConfigSave(); });
  server_.on("/generate_204", HTTP_GET,
             [this]() { sendText(204, "text/plain", ""); });
  server_.on("/gen_204", HTTP_GET,
             [this]() { sendText(204, "text/plain", ""); });
  server_.on("/hotspot-detect.html", HTTP_GET,
             [this]() { sendText(200, "text/html", "<HTML><BODY>Success</BODY></HTML>"); });
  server_.on("/connecttest.txt", HTTP_GET,
             [this]() { sendText(200, "text/plain", "Microsoft Connect Test"); });
  server_.on("/redirect", HTTP_GET, [this]() {
    server_.sendHeader("Location", "http://192.168.4.1/motor", true);
    sendText(302, "text/plain", "");
  });
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
.downloadLink{display:none;min-height:40px;border:1px solid #188038;background:#e6f4ea;color:#188038;border-radius:8px;text-decoration:none;font-size:15px;font-weight:700;align-items:center;justify-content:center}
.waveformCard{display:none;background:#fff;border:1px solid #d5d9df;border-radius:8px;padding:10px;margin:10px 0}
.waveformCard canvas{display:block;width:100%;height:96px;background:#0b0f14;border-radius:6px}
.waveformMeta{display:flex;justify-content:space-between;gap:8px;margin-top:6px;font-size:13px;color:#5f6368}
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
<div class="statusItem"><span data-i18n="batteryLevel">Battery</span><span id="batteryLevelStatus" class="statusValue">--</span></div>
<div class="statusItem"><span data-i18n="system">System</span><span id="systemStatus" class="statusValue">--</span></div>
</div>

<h2 data-i18n="quickChecks">Quick Checks</h2>
<div class="grid">
<button onclick="diag('selftest')" data-i18n="selfTest">Self Test</button>
<button onclick="diag('battery')" data-i18n="battery">Battery</button>
<button onclick="diag('mic')" data-i18n="mic">Mic</button>
<button onclick="diag('speaker')" data-i18n="speaker">Speaker</button>
<button id="audioLoopbackButton" class="primary" onclick="startAudioLoopback()" data-i18n="audioLoopback">Record Mic</button>
<button onclick="diag('i2c scan')" data-i18n="i2cScan">I2C Scan</button>
<button onclick="diag('imu')" data-i18n="imu">IMU</button>
</div>
<div id="audioDownloads" class="grid" style="display:none;margin-top:8px">
<a class="downloadLink audioDownloadLink" href="/api/audio/recording.wav" download="tongdou_mic_recording.wav">Download WAV</a>
</div>
<pre id="last" data-idle="1">Tap a test button to run it.</pre>
<div id="audioWaveformCard" class="waveformCard">
<canvas id="audioWaveform" width="640" height="96"></canvas>
<div class="waveformMeta"><span id="audioWaveformText">peak: 0</span><span data-i18n="waveform">Mic waveform</span></div>
</div>

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
<label><input id="leftInv" type="checkbox"> <span data-i18n="leftReversed">Left reversed</span></label>
<label><input id="rightInv" type="checkbox"> <span data-i18n="rightReversed">Right reversed</span></label>
</div>
<div class="row">
<label><span data-i18n="leftPwm">Left PWM</span> <input id="leftPwm" type="number" min="0" max="255"></label>
<label><span data-i18n="rightPwm">Right PWM</span> <input id="rightPwm" type="number" min="0" max="255"></label>
</div>
<div class="grid">
<button class="primary" onclick="saveMotorConfig()" data-i18n="saveMotor">Save motor setup</button>
<button class="danger" onclick="diag('motor stop')" data-i18n="stop">Stop</button>
<button onclick="diag('motor forward')" data-i18n="pulseForward">Pulse forward</button>
<button onclick="diag('motor reverse')" data-i18n="pulseReverse">Pulse reverse</button>
<button onclick="manualMotor('forward')" data-i18n="manualForward">Manual forward</button>
<button onclick="manualMotor('reverse')" data-i18n="manualReverse">Manual reverse</button>
<button onclick="gyroStraight()" data-i18n="gyroAuto">Gyro auto straight</button>
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
let audioLoopbackTimer=null;
let audioLoopbackInProgress=false;
let audioWaveform=[];
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
    batteryLevel:'Battery',
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
    audioLoopback:'Record Mic',
    waveform:'Mic waveform',
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
    soundLight:'Sound And Light',
    volume:'Speaker volume',
    applyVolume:'Apply volume',
    red:'Red',
    green:'Green',
    blue:'Blue',
    off:'Off',
    motorTitle:'Motor Direction And Power',
    leftReversed:'Left reversed',
    rightReversed:'Right reversed',
    leftPwm:'Left PWM',
    rightPwm:'Right PWM',
    saveMotor:'Save motor setup',
    stop:'Stop',
    pulseForward:'Pulse forward',
    pulseReverse:'Pulse reverse',
    manualForward:'Manual forward',
    manualReverse:'Manual reverse',
    gyroAuto:'Gyro auto straight',
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
    audioLoopback:'录音',
    waveform:'麦克风波形',
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
    soundLight:'声音与灯光',
    volume:'喇叭音量',
    applyVolume:'应用音量',
    red:'红灯',
    green:'绿灯',
    blue:'蓝灯',
    off:'关闭',
    motorTitle:'电机方向和力度',
    leftReversed:'左轮反向',
    rightReversed:'右轮反向',
    leftPwm:'左轮 PWM',
    rightPwm:'右轮 PWM',
    saveMotor:'保存电机设置',
    stop:'停止',
    pulseForward:'短促前进',
    pulseReverse:'短促后退',
    manualForward:'手动前进',
    manualReverse:'手动后退',
    gyroAuto:'陀螺仪自动直行',
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
function readMetric(text,key){
  const match=text.match(new RegExp('^\\s*'+key+'=(-?\\d+)','m'));
  return match?Number(match[1]):0;
}
function resetAudioWaveform(){
  audioWaveform=[];
  $('audioWaveformCard').style.display='block';
  $('audioWaveformText').textContent='peak: 0';
  drawAudioWaveform();
}
function setAudioDownloadsVisible(visible){
  $('audioDownloads').style.display=visible?'grid':'none';
  document.querySelectorAll('.audioDownloadLink').forEach(link=>{
    link.style.display=visible?'flex':'none';
  });
}
function pushAudioWaveform(statusText){
  const peak=readMetric(statusText,'live_peak')||readMetric(statusText,'peak');
  if(audioWaveform.length>80)audioWaveform.shift();
  audioWaveform.push(Math.max(0,Math.min(32767,peak)));
  $('audioWaveformText').textContent='peak: '+peak+'  avg: '+readMetric(statusText,'live_avg_abs');
  drawAudioWaveform();
}
function drawAudioWaveform(){
  const canvas=$('audioWaveform');
  const ctx=canvas.getContext('2d');
  const w=canvas.width;
  const h=canvas.height;
  ctx.fillStyle='#0b0f14';
  ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#24313f';
  ctx.beginPath();
  ctx.moveTo(0,h/2);
  ctx.lineTo(w,h/2);
  ctx.stroke();
  if(audioWaveform.length===0)return;
  const barWidth=Math.max(3,Math.floor(w/80)-1);
  const gap=2;
  const start=Math.max(0,w-audioWaveform.length*(barWidth+gap));
  for(let i=0;i<audioWaveform.length;i++){
    const normalized=Math.min(1,audioWaveform[i]/12000);
    const barHeight=Math.max(2,normalized*(h-12));
    const x=start+i*(barWidth+gap);
    const y=(h-barHeight)/2;
    ctx.fillStyle=normalized>.75?'#f29900':'#20c997';
    ctx.fillRect(x,y,barWidth,barHeight);
  }
}
function updateRadarPanel(data){
  radarNoticeKey=null;
  lastRadarData=data;
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
function stopAudioLoopbackPoll(){
  if(audioLoopbackTimer){
    clearInterval(audioLoopbackTimer);
    audioLoopbackTimer=null;
  }
}
function finishAudioLoopback(result){
  stopAudioLoopbackPoll();
  audioLoopbackInProgress=false;
  $('audioLoopbackButton').disabled=false;
  pushAudioWaveform(result);
  $('last').textContent='$ audio record status\n'+result;
  append('$ audio record status\n'+result);
  if(result.includes('done=1')&&result.includes('download_path=/api/audio/recording.wav')){
    setAudioDownloadsVisible(true);
  }
  refreshStatus();
}
async function pollAudioLoopback(){
  if(!audioLoopbackInProgress)return;
  try{
    const result=await post('/api/diagnostic','command=audio%20record%20status');
    $('last').textContent='$ audio record status\n'+result;
    pushAudioWaveform(result);
    if(result.includes('done=1')||result.includes('failed=1')){
      finishAudioLoopback(result);
    }
  }catch(error){
    finishAudioLoopback(text[currentLanguage].requestFailed+': '+error);
  }
}
async function startAudioLoopback(){
  stopRadarLive();
  stopAudioLoopbackPoll();
  audioLoopbackInProgress=true;
  $('audioLoopbackButton').disabled=true;
  resetAudioWaveform();
  setAudioDownloadsVisible(false);
  $('last').dataset.idle='0';
  $('last').textContent=text[currentLanguage].running+': Record Mic ...';
  append('$ audio record\n'+text[currentLanguage].running+' ...');
  try{
    const result=await post('/api/diagnostic','command=audio%20record');
    $('last').textContent='$ audio record\n'+result;
    append('$ audio record\n'+result);
    if(result.includes('failed=1')){
      finishAudioLoopback(result);
      return;
    }
    audioLoopbackTimer=setInterval(pollAudioLoopback,400);
  }catch(error){
    finishAudioLoopback(text[currentLanguage].requestFailed+': '+error);
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
  const batteryText=data.batteryPercent+'% / '+(data.batteryVoltageMv/1000).toFixed(2)+'V';
  setStatus('batteryLevelStatus',data.batteryPercent>15,batteryText);
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

  Serial.print(F("web diagnostic command="));
  Serial.println(command);

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

void WebConfigServer::handleAudioRecordingDownload() {
  if (!hardwareSelfTest_.audioRecordingAvailable()) {
    sendText(404, "text/plain; charset=utf-8", "no recording available");
    return;
  }

  const int16_t* samples = hardwareSelfTest_.audioRecordingSamples();
  const size_t sampleCount = hardwareSelfTest_.audioRecordingSampleCount();
  if (samples == nullptr || sampleCount == 0) {
    sendText(404, "text/plain; charset=utf-8", "empty recording");
    return;
  }

  int64_t sum = 0;
  for (size_t index = 0; index < sampleCount; ++index) {
    sum += samples[index];
  }
  const int32_t mean = static_cast<int32_t>(sum / static_cast<int64_t>(sampleCount));

  int64_t sumSquares = 0;
  int32_t previousInput = 0;
  int32_t previousOutput = 0;
  for (size_t index = 0; index < sampleCount; ++index) {
    const int32_t processed =
        highPassPcm16(static_cast<int32_t>(samples[index]) - mean,
                      previousInput, previousOutput);
    sumSquares += static_cast<int64_t>(processed) * static_cast<int64_t>(processed);
  }

  float gain = 1.0F;
  const float rms = sqrtf(static_cast<float>(sumSquares) /
                         static_cast<float>(sampleCount));
  if (rms > 1.0F) {
    gain = kAudioExportTargetRms / rms;
    if (gain < 1.0F) {
      gain = 1.0F;
    } else if (gain > kAudioExportMaxGain) {
      gain = kAudioExportMaxGain;
    }
  }

  uint8_t header[kAudioWavHeaderBytes] = {};
  buildWavHeader(header, hardwareSelfTest_.audioRecordingSampleRateHz(),
                 static_cast<uint32_t>(sampleCount));

  server_.sendHeader("Cache-Control", "no-store");
  server_.sendHeader("Content-Disposition",
                     "attachment; filename=\"tongdou_mic_recording.wav\"");
  server_.setContentLength(kAudioWavHeaderBytes + sampleCount * sizeof(int16_t));
  server_.send(200, "audio/wav", "");

  WiFiClient client = server_.client();
  client.write(header, sizeof(header));

  int16_t chunk[kAudioExportChunkSamples] = {};
  previousInput = 0;
  previousOutput = 0;
  size_t offset = 0;
  while (offset < sampleCount && client.connected()) {
    const size_t count = min(kAudioExportChunkSamples, sampleCount - offset);
    for (size_t index = 0; index < count; ++index) {
      const int32_t processed =
          highPassPcm16(static_cast<int32_t>(samples[offset + index]) - mean,
                        previousInput, previousOutput);
      chunk[index] = clampPcm16(static_cast<int32_t>(processed * gain));
    }
    client.write(reinterpret_cast<const uint8_t*>(chunk),
                 count * sizeof(int16_t));
    offset += count;
  }
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
  body += ",\"stateTarget\":";
  body += boolJson(target.stateTarget);
  body += ",\"energyTarget\":";
  body += boolJson(target.energyTarget);
  body += ",\"movingGateTarget\":";
  body += boolJson(target.movingGateTarget);
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
  sendText(404, "text/plain; charset=utf-8", "not found");
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
  body += ",\"batteryVoltageMv\":";
  body += battery.voltageMv;
  body += ",\"batteryPercent\":";
  body += battery.percent;
  body += ",\"shippingDischargeActive\":";
  body += boolJson(hardwareSelfTest_.shippingDischargeActive());
  body += ",\"shippingDischargeDone\":";
  body += boolJson(hardwareSelfTest_.shippingDischargeDone());
  body += ",\"shippingDischargeFailed\":";
  body += boolJson(hardwareSelfTest_.shippingDischargeFailed());
  body += ",\"shippingDischargeTargetPercent\":";
  body += hardwareSelfTest_.shippingDischargeTargetPercent();
  body += ",\"shippingDischargeMessage\":";
  body += jsonString(hardwareSelfTest_.shippingDischargeMessage());
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
