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
constexpr unsigned long kStationDisconnectSettleMs = 120;
constexpr uint32_t kWifiScanFirstPassMsPerChannel = 220;
constexpr uint32_t kWifiScanRetryMsPerChannel = 300;

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

const char* localeDisplayName(const String& locale) {
  if (locale == "en-GB") {
    return "English UK";
  }
  if (locale == "en-CA") {
    return "English Canada";
  }
  if (locale == "fr-FR") {
    return "Français";
  }
  if (locale == "fr-CA") {
    return "Français Canada";
  }
  if (locale == "nl-NL") {
    return "Nederlands";
  }
  if (locale == "ru-RU") {
    return "Русский";
  }
  if (locale == "zh-CN") {
    return "中文";
  }
  return "English UK";
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
                                 HardwareSelfTestService& hardwareSelfTest,
                                 DeviceLocaleStore& deviceLocaleStore,
                                 LocaleResourceUpdateService&
                                     localeResourceUpdateService)
    : scenarioApi_(scenarioApi),
      wifiStore_(wifiStore),
      timeService_(timeService),
      reminderStore_(reminderStore),
      mcpServer_(mcpServer),
      hardwareSelfTest_(hardwareSelfTest),
      deviceLocaleStore_(deviceLocaleStore),
      localeResourceUpdateService_(localeResourceUpdateService) {}

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
      wifiConnectedEvent_ = true;
      timeService_.configureNetworkTime();
      Serial.print("wifi connected ip=");
      Serial.println(WiFi.localIP());
    } else if (millis() - connectStartedMs_ > kConnectTimeoutMs) {
      Serial.println("wifi connect timeout, starting setup portal");
      wifiConnectFailedEvent_ = true;
      startPortal();
    }
  }

  timeService_.update();
}

ScenarioConfigApi& WebConfigServer::scenarioApi() {
  return scenarioApi_;
}

WebConfigEvent WebConfigServer::consumeEvent() {
  if (wifiConnectFailedEvent_) {
    wifiConnectFailedEvent_ = false;
    return WebConfigEvent::WifiConnectFailed;
  }
  if (wifiConnectedEvent_) {
    wifiConnectedEvent_ = false;
    return WebConfigEvent::WifiConnected;
  }
  if (localeChangedEvent_) {
    localeChangedEvent_ = false;
    return WebConfigEvent::LocaleChanged;
  }
  if (setupPortalStartedEvent_) {
    setupPortalStartedEvent_ = false;
    return WebConfigEvent::SetupPortalStarted;
  }
  return WebConfigEvent::None;
}

void WebConfigServer::startPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(false, false);
  delay(kStationDisconnectSettleMs);
  WiFi.softAPConfig(kPortalIp, kPortalGateway, kPortalSubnet);
  WiFi.softAP(kPortalSsid);

  dnsServer_.start(kDnsPort, "*", kPortalIp);
  dnsStarted_ = true;
  state_ = NetworkState::Portal;
  setupPortalStartedEvent_ = true;

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
  server_.on("/favicon.ico", HTTP_GET, [this]() { sendText(204, "text/plain", ""); });
  server_.on("/generate_204", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/gen_204", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/hotspot-detect.html", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/connecttest.txt", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/status", HTTP_GET, [this]() { sendText(200, "application/json", statusJson()); });
  server_.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
  server_.on("/api/setup/wifi", HTTP_POST, [this]() { handleWifiSave(); });
  server_.on("/api/locale", HTTP_GET, [this]() { handleLocaleGet(); });
  server_.on("/api/locale", HTTP_POST, [this]() { handleLocaleSave(); });
  server_.on("/api/time/sync", HTTP_POST, [this]() { handleTimeSync(); });
  server_.on("/api/time/status", HTTP_GET, [this]() { handleTimeStatus(); });
  server_.on("/api/reminders", HTTP_GET, [this]() { handleReminderList(); });
  server_.on("/api/reminders", HTTP_POST, [this]() { handleReminderCreate(); });
  server_.on("/api/reminders/delete", HTTP_POST, [this]() { handleReminderDelete(); });
  server_.on("/api/diagnostic", HTTP_POST, [this]() { handleDiagnosticCommand(); });
  server_.on("/api/mcp", HTTP_POST, [this]() { handleMcp(); });
  server_.on("/voice", HTTP_GET, [this]() { handleVoiceDebugPage(); });
  server_.on("/api/voice/status", HTTP_GET, [this]() { handleVoiceStatus(); });
  server_.on("/api/voice/backend", HTTP_POST, [this]() { handleVoiceBackendConfigure(); });
  server_.on("/api/voice/connect", HTTP_POST, [this]() { handleVoiceBackendConnect(); });
  server_.on("/api/voice/detect", HTTP_POST, [this]() { handleVoiceDetect(); });
  server_.on("/api/voice/start", HTTP_POST, [this]() { handleVoiceStart(); });
  server_.on("/api/voice/start-auto", HTTP_POST,
             [this]() { handleVoiceStartAuto(); });
  server_.on("/api/voice/abort", HTTP_POST, [this]() { handleVoiceAbort(); });
  server_.on("/api/voice/exit", HTTP_POST, [this]() { handleVoiceExit(); });
  server_.on("/api/voice/codec-self-test", HTTP_POST,
             [this]() { handleVoiceCodecSelfTest(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void WebConfigServer::handleRoot() {
  {
    String body;
    body.reserve(18000);
    body += F(R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>铜豆控制台</title>
<style>
body{font-family:Arial,'Microsoft YaHei',sans-serif;margin:0;background:#121212;color:#f4f1ea}
main{max-width:980px;margin:auto;padding:22px}
h1{margin:0 0 6px}h2{margin:0 0 12px}h3{margin:16px 0 8px}.sub{color:#aaa;margin:0 0 18px}
.card{background:#1d1d1d;padding:18px;border-radius:8px;margin:14px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px}
.status{display:grid;grid-template-columns:repeat(auto-fit,minmax(155px,1fr));gap:8px}
.item{background:#0f0f0f;border:1px solid #2a2a2a;border-radius:8px;padding:12px}
.label{color:#aaa;font-size:12px}.value{font-size:17px;margin-top:4px;word-break:break-word}
button,input,select{box-sizing:border-box;width:100%;padding:12px;margin:5px 0;border-radius:8px}
button{border:0;background:#e7b45f;color:#111;font-weight:700;cursor:pointer}button.secondary{background:#2a2a2a;color:#f4f1ea}
input,select{background:#0f0f0f;color:#f4f1ea;border:1px solid #2a2a2a}
pre{white-space:pre-wrap;background:#0b0b0b;padding:12px;border-radius:8px;min-height:120px}
.meter{height:16px;background:#0b0b0b;border:1px solid #2a2a2a;border-radius:999px;overflow:hidden}.bar{height:100%;width:0;background:#e7b45f}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}@media(max-width:560px){.row{grid-template-columns:1fr}}
small{color:#aaa}
</style></head><body><main>
<h1 data-i18n="appTitle">铜豆控制台</h1><p class="sub" data-i18n="appSub">本地配置、状态查看和硬件诊断。</p>

<section class="card"><h2 data-i18n="language">语言</h2>
<select id="locale" onchange="saveLocale()">
<option value="en-GB">English UK</option>
<option value="en-CA">English Canada</option>
<option value="fr-FR">Français</option>
<option value="fr-CA">Français Canada</option>
<option value="nl-NL">Nederlands</option>
<option value="ru-RU">Русский</option>
<option value="zh-CN">中文</option>
</select>
<small id="localeMsg" data-i18n="languageHelp">提示音、验证码播报和配置页面会使用这个语言。</small></section>

<section class="card"><h2 data-i18n="status">状态</h2><div class="status">
<div class="item"><div class="label" data-i18n="network">网络</div><div class="value" id="wifi" data-i18n="loading">读取中</div></div>
<div class="item"><div class="label" data-i18n="address">地址</div><div class="value" id="ip" data-i18n="loading">读取中</div></div>
<div class="item"><div class="label" data-i18n="time">时间</div><div class="value" id="time" data-i18n="loading">读取中</div></div>
<div class="item"><div class="label" data-i18n="language">语言</div><div class="value" id="localeStatus" data-i18n="loading">读取中</div></div>
<div class="item"><div class="label" data-i18n="battery">电池</div><div class="value" id="battery" data-i18n="loading">读取中</div></div>
<div class="item"><div class="label" data-i18n="hardware">硬件</div><div class="value" id="hardware" data-i18n="loading">读取中</div></div>
<div class="item"><div class="label" data-i18n="capability">能力</div><div class="value" id="capability" data-i18n="loading">读取中</div></div>
</div><button onclick="loadStatus()" data-i18n="refreshStatus">刷新状态</button></section>

<section class="card"><h2 data-i18n="connectionTime">连接和时间</h2>
<form id="wifiForm" onsubmit="saveWifi(event)">
<input id="ssid" name="ssid" list="wifiOptions" placeholder="WiFi 名称" data-i18n-placeholder="wifiName" required>
<datalist id="wifiOptions"></datalist>
<input id="password" name="password" type="password" placeholder="WiFi 密码" data-i18n-placeholder="wifiPassword">
<div class="row"><button id="scanBtn" type="button" class="secondary" onclick="scanWifi()" data-i18n="scanWifi">扫描附近 WiFi</button><button type="submit" data-i18n="saveConnect">保存并连接</button></div>
</form><div id="wifiList" class="grid"></div><button onclick="syncTime()" data-i18n="syncTime">同步当前时间</button><small id="msg"></small></section>

<section class="card"><h2 data-i18n="microphone">麦克风</h2>
<div class="meter"><div class="bar" id="micBar"></div></div>
<div class="row"><button onclick="startMicTest()" data-i18n="startTest">开始持续测试</button><button class="secondary" onclick="stopMicTest()" data-i18n="stop">停止</button></div>
<div class="item"><div class="label" data-i18n="currentReading">当前读数</div><div class="value" id="micValue" data-i18n="notStarted">未开始</div></div></section>

<section class="card"><h2 data-i18n="radar">雷达</h2>
<div class="status">
<div class="item"><div class="label" data-i18n="occupied">是否有人</div><div class="value" id="radarOccupied" data-i18n="notRead">未读取</div></div>
<div class="item"><div class="label" data-i18n="distance">距离</div><div class="value" id="radarDistance" data-i18n="distanceUnused">不用于判断</div></div>
<div class="item"><div class="label" data-i18n="rawAux">原始辅助值</div><div class="value" id="radarRaw" data-i18n="notRead">未读取</div></div>
<div class="item"><div class="label" data-i18n="state">状态</div><div class="value" id="radarState" data-i18n="notRead">未读取</div></div>
</div>
<div class="row"><button onclick="startRadarTest()" data-i18n="startTest">开始持续测试</button><button class="secondary" onclick="stopRadarTest()" data-i18n="stop">停止</button></div></section>

<section class="card"><h2 data-i18n="outputTest">输出测试</h2><div class="grid">
<button onclick="diag('speaker')" data-i18n="speakerTone">喇叭短音</button>
<button onclick="diag('audio cue success')" data-i18n="successCue">成功提示</button>
<button onclick="diag('audio cue error')" data-i18n="errorCue">错误提示</button>
<button onclick="diag('audio cue wifi '+$('locale').value)" data-i18n="wifiCue">配网提示</button>
<button onclick="diag('audio code 1863 '+$('locale').value)" data-i18n="codeCue">验证码播报</button>
<button onclick="diag('led red')" data-i18n="redLed">红灯</button>
<button onclick="diag('led green')" data-i18n="greenLed">绿灯</button>
<button onclick="diag('led blue')" data-i18n="blueLed">蓝灯</button>
<button onclick="diag('led off')" data-i18n="ledOff">关灯</button>
<button onclick="diag('motor fault')" data-i18n="motorFault">电机故障</button>
<button onclick="diag('motor fault simulate on')" data-i18n="motorFaultOn">模拟电机故障</button>
<button onclick="diag('motor fault simulate off')" data-i18n="motorFaultOff">取消模拟故障</button>
<button onclick="diag('motor forward')" data-i18n="motorForward">电机正转</button>
<button onclick="diag('motor reverse')" data-i18n="motorReverse">电机反转</button>
<button onclick="diag('motor turn left')" data-i18n="motorLeft">电机左扭</button>
<button onclick="diag('motor turn right')" data-i18n="motorRight">电机右扭</button>
<button onclick="diag('motor stop')" data-i18n="motorStop">电机停止</button>
</div></section>

<section class="card"><h2 data-i18n="hardwareRead">硬件读取</h2><div class="grid">
<button onclick="diag('selftest')" data-i18n="fullSelftest">完整自检</button>
<button onclick="diag('status')" data-i18n="hardwareStatus">硬件状态</button>
<button onclick="diag('battery')" data-i18n="batteryCharge">电池/充电</button>
<button onclick="diag('i2c scan')" data-i18n="i2cScan">I2C 扫描</button>
<button onclick="diag('imu')">QMI8658A</button>
<button onclick="diag('logo touch')" data-i18n="logoTouch">Logo 触摸</button>
<button onclick="diag('radar')" data-i18n="radarLevel">雷达电平</button>
<button onclick="diag('radar target')" data-i18n="radarTarget">雷达目标</button>
<button onclick="diag('radar sample')" data-i18n="radarSample">雷达连续采样</button>
<button onclick="diag('mic')" data-i18n="micOnce">麦克风单次</button>
</div></section>

<section class="card"><h2 data-i18n="scenarioTest">剧情测试</h2><div class="grid">
<button onclick="mcpTool('play_scenario',{event:'boot_completed'})" data-i18n="bootScenario">开机场景</button>
<button onclick="mcpTool('play_scenario',{event:'voice_recognition_failed'})" data-i18n="recognitionFailed">识别失败</button>
<button onclick="mcpTool('play_scenario',{event:'logo_single_tapped'})" data-i18n="logoSingle">Logo 单击</button>
<button onclick="mcpTool('play_scenario',{event:'logo_double_tapped'})" data-i18n="logoDouble">Logo 双击</button>
<button onclick="mcpTool('play_scenario',{event:'logo_long_pressed'})" data-i18n="logoLong">Logo 长按</button>
<button onclick="mcpTool('voice_status',{})" data-i18n="voiceStatus">语音状态</button>
</div></section>

<section class="card"><h2 data-i18n="output">输出</h2><pre id="diag" data-i18n="waiting">等待操作</pre></section>

<script>
const $=id=>document.getElementById(id);let micTimer=null,radarTimer=null;
const I18N={
'zh-CN':{appTitle:'铜豆控制台',appSub:'本地配置、状态查看和硬件诊断。',language:'语言',languageHelp:'提示音、验证码播报和配置页面会使用这个语言。',status:'状态',network:'网络',address:'地址',time:'时间',battery:'电池',hardware:'硬件',capability:'能力',loading:'读取中',refreshStatus:'刷新状态',connectionTime:'连接和时间',wifiName:'WiFi 名称',wifiPassword:'WiFi 密码',scanWifi:'扫描附近 WiFi',saveConnect:'保存并连接',syncTime:'同步当前时间',microphone:'麦克风',startTest:'开始持续测试',stop:'停止',currentReading:'当前读数',notStarted:'未开始',radar:'雷达',occupied:'是否有人',distance:'距离',rawAux:'原始辅助值',state:'状态',notRead:'未读取',distanceUnused:'不用于判断',outputTest:'输出测试',speakerTone:'喇叭短音',successCue:'成功提示',errorCue:'错误提示',wifiCue:'配网提示',codeCue:'验证码播报',redLed:'红灯',greenLed:'绿灯',blueLed:'蓝灯',ledOff:'关灯',motorFault:'电机故障',motorFaultOn:'模拟电机故障',motorFaultOff:'取消模拟故障',motorForward:'电机正转',motorReverse:'电机反转',motorLeft:'电机左扭',motorRight:'电机右扭',motorStop:'电机停止',hardwareRead:'硬件读取',fullSelftest:'完整自检',hardwareStatus:'硬件状态',batteryCharge:'电池/充电',i2cScan:'I2C 扫描',logoTouch:'Logo 触摸',radarLevel:'雷达电平',radarTarget:'雷达目标',radarSample:'雷达连续采样',micOnce:'麦克风单次',scenarioTest:'剧情测试',bootScenario:'开机场景',recognitionFailed:'识别失败',logoSingle:'Logo 单击',logoDouble:'Logo 双击',logoLong:'Logo 长按',voiceStatus:'语音状态',output:'输出',waiting:'等待操作',wifiConnected:'已连接',portalMode:'配网模式',connecting:'连接中',timeNotReady:'未校时',readFailed:'读取失败',usb:'USB',yes:'是',no:'否',charging:'充电',normal:'正常',degraded:'降级',reminders:'提醒',web:'网页',visual:'视觉',sound:'声音',voiceInput:'麦克风',none:'无',savingLanguage:'正在保存语言',scanStart:'正在扫描附近 WiFi，可能需要几秒',scanEmpty:'没有扫到 WiFi，铜豆已经自动重试过一次，可以再点扫描或手动填写',scanDone:'点一个 WiFi 名称，或继续手动填写',scanFailed:'扫描失败，请再试一次，或手动填写',missingSsid:'请先选择或填写 WiFi 名称',savingWifi:'正在保存并连接',running:'执行中：',unknown:'未知',present:'有人',absent:'无人',radarUnused:'不可用：LD2412 只用于有人/无人',movingRaw:'移动原始',energy:'能量',staticRaw:'静止原始',radar0:'无人',radar1:'移动目标',radar2:'静止目标',radar3:'移动+静止'},
'en-GB':{appTitle:'Tong Dou Console',appSub:'Local setup, status, and hardware diagnostics.',language:'Language',languageHelp:'This language is used for prompts, activation codes, and this page.',status:'Status',network:'Network',address:'Address',time:'Time',battery:'Battery',hardware:'Hardware',capability:'Capabilities',loading:'Loading',refreshStatus:'Refresh status',connectionTime:'Connection and time',wifiName:'WiFi name',wifiPassword:'WiFi password',scanWifi:'Scan nearby WiFi',saveConnect:'Save and connect',syncTime:'Sync current time',microphone:'Microphone',startTest:'Start continuous test',stop:'Stop',currentReading:'Current reading',notStarted:'Not started',radar:'Radar',occupied:'Presence',distance:'Distance',rawAux:'Raw auxiliary values',state:'State',notRead:'Not read',distanceUnused:'Not used for decisions',outputTest:'Output test',speakerTone:'Speaker tone',successCue:'Success prompt',errorCue:'Error prompt',wifiCue:'Setup prompt',codeCue:'Activation code prompt',redLed:'Red light',greenLed:'Green light',blueLed:'Blue light',ledOff:'Light off',motorFault:'Motor fault',motorFaultOn:'Simulate motor fault',motorFaultOff:'Clear simulated fault',motorForward:'Motor forward',motorReverse:'Motor reverse',motorLeft:'Motor twist left',motorRight:'Motor twist right',motorStop:'Motor stop',hardwareRead:'Hardware readings',fullSelftest:'Full self-test',hardwareStatus:'Hardware status',batteryCharge:'Battery/charging',i2cScan:'I2C scan',logoTouch:'Logo touch',radarLevel:'Radar level',radarTarget:'Radar target',radarSample:'Radar samples',micOnce:'Microphone once',scenarioTest:'Scenario test',bootScenario:'Boot scenario',recognitionFailed:'Recognition failed',logoSingle:'Logo single tap',logoDouble:'Logo double tap',logoLong:'Logo long press',voiceStatus:'Voice status',output:'Output',waiting:'Waiting',wifiConnected:'Connected',portalMode:'Setup mode',connecting:'Connecting',timeNotReady:'Time not set',readFailed:'Read failed',usb:'USB',yes:'yes',no:'no',charging:'charging',normal:'Normal',degraded:'Degraded',reminders:'Reminders',web:'Web',visual:'Visual',sound:'Sound',voiceInput:'Microphone',none:'None',savingLanguage:'Saving language',scanStart:'Scanning nearby WiFi. This may take a few seconds.',scanEmpty:'No WiFi found. Tong Dou already retried once. Scan again or enter it manually.',scanDone:'Pick a WiFi name, or enter one manually.',scanFailed:'Scan failed. Try again or enter it manually.',missingSsid:'Choose or enter a WiFi name first.',savingWifi:'Saving and connecting',running:'Running: ',unknown:'Unknown',present:'Present',absent:'Absent',radarUnused:'Not used: LD2412 is only used for presence.',movingRaw:'moving raw',energy:'energy',staticRaw:'static raw',radar0:'No target',radar1:'Moving target',radar2:'Static target',radar3:'Moving + static'},
'en-CA':null,'fr-FR':null,'fr-CA':null,'nl-NL':null,'ru-RU':null};
I18N['en-CA']=I18N['en-GB'];
I18N['fr-FR']={...I18N['en-GB'],appTitle:'Console Tong Dou',appSub:'Configuration locale, état et diagnostic matériel.',language:'Langue',languageHelp:'Cette langue est utilisée pour les annonces, le code d’activation et cette page.',status:'État',network:'Réseau',address:'Adresse',time:'Heure',battery:'Batterie',hardware:'Matériel',capability:'Capacités',loading:'Chargement',refreshStatus:'Actualiser',connectionTime:'Connexion et heure',wifiName:'Nom du WiFi',wifiPassword:'Mot de passe WiFi',scanWifi:'Scanner les WiFi proches',saveConnect:'Enregistrer et connecter',syncTime:'Synchroniser l’heure',microphone:'Microphone',startTest:'Démarrer le test continu',stop:'Arrêter',currentReading:'Lecture actuelle',notStarted:'Pas démarré',radar:'Radar',occupied:'Présence',distance:'Distance',rawAux:'Valeurs brutes',state:'État',notRead:'Non lu',distanceUnused:'Non utilisé pour décider',outputTest:'Test de sortie',speakerTone:'Son haut-parleur',successCue:'Annonce de succès',errorCue:'Annonce d’erreur',wifiCue:'Annonce de configuration',codeCue:'Annonce du code',hardwareRead:'Lecture matériel',fullSelftest:'Autotest complet',hardwareStatus:'État matériel',batteryCharge:'Batterie/charge',logoTouch:'Toucher logo',radarLevel:'Niveau radar',radarTarget:'Cible radar',radarSample:'Échantillons radar',micOnce:'Micro une fois',scenarioTest:'Test scénario',bootScenario:'Scénario démarrage',recognitionFailed:'Reconnaissance échouée',voiceStatus:'État voix',output:'Sortie',waiting:'En attente',wifiConnected:'Connecté',portalMode:'Mode configuration',connecting:'Connexion',timeNotReady:'Heure non réglée',readFailed:'Lecture échouée',yes:'oui',no:'non',charging:'charge',normal:'Normal',degraded:'Dégradé',reminders:'Rappels',visual:'Visuel',sound:'Son',voiceInput:'Microphone',none:'Aucun',savingLanguage:'Enregistrement de la langue',scanStart:'Recherche des WiFi proches. Cela peut prendre quelques secondes.',scanEmpty:'Aucun WiFi trouvé. Tong Dou a déjà réessayé. Relancez ou saisissez manuellement.',scanDone:'Choisissez un WiFi ou saisissez-le manuellement.',scanFailed:'Échec du scan. Réessayez ou saisissez manuellement.',missingSsid:'Choisissez ou saisissez d’abord un nom WiFi.',savingWifi:'Enregistrement et connexion',running:'Exécution : ',unknown:'Inconnu',present:'Présent',absent:'Absent',radarUnused:'Non utilisé : le LD2412 sert seulement à détecter une présence.'};
I18N['fr-CA']=I18N['fr-FR'];
I18N['nl-NL']={...I18N['en-GB'],appTitle:'Tong Dou-console',appSub:'Lokale configuratie, status en hardwarediagnose.',language:'Taal',languageHelp:'Deze taal wordt gebruikt voor meldingen, activatiecodes en deze pagina.',status:'Status',network:'Netwerk',address:'Adres',time:'Tijd',battery:'Batterij',hardware:'Hardware',capability:'Mogelijkheden',loading:'Laden',refreshStatus:'Status vernieuwen',connectionTime:'Verbinding en tijd',wifiName:'WiFi-naam',wifiPassword:'WiFi-wachtwoord',scanWifi:'WiFi in de buurt zoeken',saveConnect:'Opslaan en verbinden',syncTime:'Huidige tijd synchroniseren',microphone:'Microfoon',startTest:'Continue test starten',stop:'Stoppen',currentReading:'Huidige waarde',notStarted:'Niet gestart',radar:'Radar',occupied:'Aanwezigheid',distance:'Afstand',rawAux:'Ruwe hulpwaarden',state:'Status',notRead:'Niet gelezen',distanceUnused:'Niet gebruikt voor beslissing',outputTest:'Uitvoertest',speakerTone:'Luidsprekertoon',successCue:'Succesmelding',errorCue:'Foutmelding',wifiCue:'Configuratiemelding',codeCue:'Activatiecode voorlezen',hardwareRead:'Hardwarewaarden',fullSelftest:'Volledige zelftest',hardwareStatus:'Hardwarestatus',batteryCharge:'Batterij/laden',logoTouch:'Logo-aanraking',radarLevel:'Radarniveau',radarTarget:'Radardoel',radarSample:'Radarsamples',micOnce:'Microfoon eenmalig',scenarioTest:'Scenariotest',bootScenario:'Opstartscenario',recognitionFailed:'Herkenning mislukt',voiceStatus:'Spraakstatus',output:'Uitvoer',waiting:'Wachten',wifiConnected:'Verbonden',portalMode:'Configuratiemodus',connecting:'Verbinden',timeNotReady:'Tijd niet ingesteld',readFailed:'Lezen mislukt',yes:'ja',no:'nee',charging:'laden',normal:'Normaal',degraded:'Beperkt',reminders:'Herinneringen',visual:'Visueel',sound:'Geluid',voiceInput:'Microfoon',none:'Geen',savingLanguage:'Taal opslaan',scanStart:'WiFi in de buurt zoeken. Dit kan een paar seconden duren.',scanEmpty:'Geen WiFi gevonden. Tong Dou heeft al opnieuw geprobeerd. Scan opnieuw of vul handmatig in.',scanDone:'Kies een WiFi-naam of vul handmatig in.',scanFailed:'Scannen mislukt. Probeer opnieuw of vul handmatig in.',missingSsid:'Kies of vul eerst een WiFi-naam in.',savingWifi:'Opslaan en verbinden',running:'Bezig: ',unknown:'Onbekend',present:'Aanwezig',absent:'Afwezig',radarUnused:'Niet gebruikt: LD2412 wordt alleen gebruikt voor aanwezigheid.'};
I18N['ru-RU']={...I18N['en-GB'],appTitle:'Панель Tong Dou',appSub:'Локальная настройка, состояние и диагностика.',language:'Язык',languageHelp:'Этот язык используется для подсказок, кода активации и этой страницы.',status:'Состояние',network:'Сеть',address:'Адрес',time:'Время',battery:'Батарея',hardware:'Оборудование',capability:'Возможности',loading:'Загрузка',refreshStatus:'Обновить состояние',connectionTime:'Подключение и время',wifiName:'Имя WiFi',wifiPassword:'Пароль WiFi',scanWifi:'Сканировать WiFi рядом',saveConnect:'Сохранить и подключить',syncTime:'Синхронизировать время',microphone:'Микрофон',startTest:'Начать непрерывный тест',stop:'Стоп',currentReading:'Текущее значение',notStarted:'Не начато',radar:'Радар',occupied:'Присутствие',distance:'Расстояние',rawAux:'Сырые значения',state:'Состояние',notRead:'Не прочитано',distanceUnused:'Не используется для решения',outputTest:'Тест вывода',speakerTone:'Короткий звук',successCue:'Сигнал успеха',errorCue:'Сигнал ошибки',wifiCue:'Подсказка настройки',codeCue:'Озвучить код',hardwareRead:'Данные оборудования',fullSelftest:'Полная самопроверка',hardwareStatus:'Состояние оборудования',batteryCharge:'Батарея/заряд',logoTouch:'Касание логотипа',radarLevel:'Уровень радара',radarTarget:'Цель радара',radarSample:'Сэмплы радара',micOnce:'Микрофон один раз',scenarioTest:'Тест сценария',bootScenario:'Сценарий запуска',recognitionFailed:'Ошибка распознавания',voiceStatus:'Состояние голоса',output:'Вывод',waiting:'Ожидание',wifiConnected:'Подключено',portalMode:'Режим настройки',connecting:'Подключение',timeNotReady:'Время не задано',readFailed:'Ошибка чтения',yes:'да',no:'нет',charging:'заряд',normal:'Норма',degraded:'Ограничено',reminders:'Напоминания',visual:'Визуально',sound:'Звук',voiceInput:'Микрофон',none:'Нет',savingLanguage:'Сохранение языка',scanStart:'Сканирование WiFi рядом. Это может занять несколько секунд.',scanEmpty:'WiFi не найден. Tong Dou уже повторил попытку. Сканируйте снова или введите вручную.',scanDone:'Выберите WiFi или введите вручную.',scanFailed:'Сканирование не удалось. Повторите или введите вручную.',missingSsid:'Сначала выберите или введите имя WiFi.',savingWifi:'Сохранение и подключение',running:'Выполняется: ',unknown:'Неизвестно',present:'Есть',absent:'Нет',radarUnused:'Не используется: LD2412 только определяет присутствие.'};
let currentLocale='en-GB';
function tr(k){let d=I18N[currentLocale]||I18N['en-GB'];return (d&&d[k])||I18N['en-GB'][k]||I18N['zh-CN'][k]||k}
function applyI18n(){document.documentElement.lang=currentLocale;document.title=tr('appTitle');document.querySelectorAll('[data-i18n]').forEach(e=>e.textContent=tr(e.dataset.i18n));document.querySelectorAll('[data-i18n-placeholder]').forEach(e=>e.placeholder=tr(e.dataset.i18nPlaceholder))}
function localeName(v){return {'en-GB':'English UK','en-CA':'English Canada','fr-FR':'Français','fr-CA':'Français Canada','nl-NL':'Nederlands','ru-RU':'Русский','zh-CN':'中文'}[v]||v}
function capText(c){let a=[];if(c.reminders)a.push(tr('reminders'));if(c.web)a.push(tr('web'));if(c.visual)a.push(tr('visual'));if(c.sound)a.push(tr('sound'));if(c.voiceInput)a.push(tr('voiceInput'));return a.join(' / ')||tr('none')}
async function loadStatus(){try{let r=await fetch('/api/status');let j=await r.json();currentLocale=j.locale||currentLocale;if($('locale').value!==currentLocale)$('locale').value=currentLocale;applyI18n();$('wifi').textContent=j.wifiConnected?tr('wifiConnected'):(j.portal?tr('portalMode'):tr('connecting'));$('ip').textContent=j.ip||'-';$('time').textContent=j.timeReady?j.now:tr('timeNotReady');$('localeStatus').textContent=localeName(currentLocale);$('battery').textContent=`${tr('usb')} ${j.battery.usb?tr('yes'):tr('no')} / ${tr('charging')} ${j.battery.charging?tr('yes'):tr('no')} / ADC ${j.battery.rawAdc}`;$('hardware').textContent=j.hardware.degraded?(j.hardware.motorFault?tr('motorFault'):tr('degraded')):tr('normal');$('capability').textContent=capText(j.capabilities)}catch(e){$('wifi').textContent=tr('readFailed')}}
async function saveLocale(){let v=$('locale').value;$('localeMsg').textContent=tr('savingLanguage');try{let r=await fetch('/api/locale',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'locale='+encodeURIComponent(v)});$('localeMsg').textContent=await r.text();if(r.ok){currentLocale=v;applyI18n();$('localeStatus').textContent=localeName(v)}else{$('locale').value=currentLocale}}catch(e){$('locale').value=currentLocale;$('localeMsg').textContent=tr('readFailed')}}
async function syncTime(){let e=Math.floor(Date.now()/1000);let r=await fetch('/api/time/sync',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'epoch='+e});$('msg').textContent=await r.text();loadStatus()}
async function scanWifi(){let m=$('msg'),l=$('wifiList'),d=$('wifiOptions'),btn=$('scanBtn');m.textContent=tr('scanStart');l.innerHTML='';if(btn)btn.disabled=true;try{let r=await fetch('/api/wifi/scan');let a=await r.json();d.innerHTML='';if(!a.length){m.textContent=tr('scanEmpty');return}a.sort((x,y)=>y.rssi-x.rssi);a.slice(0,16).forEach(n=>{let o=document.createElement('option');o.value=n.ssid;d.appendChild(o);let b=document.createElement('button');b.type='button';b.className='secondary';b.textContent=n.ssid+'  '+n.rssi+' dBm';b.onclick=()=>{$('ssid').value=n.ssid;$('password').focus()};l.appendChild(b)});m.textContent=tr('scanDone')}catch(e){m.textContent=tr('scanFailed')}finally{if(btn)btn.disabled=false}}
async function saveWifi(e){e.preventDefault();let s=$('ssid').value.trim(),p=$('password').value;if(!s){$('msg').textContent=tr('missingSsid');return}$('msg').textContent=tr('savingWifi');let r=await fetch('/api/setup/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(s)+'&password='+encodeURIComponent(p)});$('msg').textContent=await r.text();setTimeout(loadStatus,1200)}
async function diag(c){$('diag').textContent=tr('running')+c;let r=await fetch('/api/diagnostic',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'command='+encodeURIComponent(c)});let t=await r.text();$('diag').textContent=t;return t}
function val(t,k){let m=t.match(new RegExp(k+'=([^\\n\\r]+)'));return m?m[1].trim():''}
async function pollMic(){let t=await diag('mic');let avg=Number(val(t,'avg_abs')||0),peak=Number(val(t,'peak')||0);$('micValue').textContent=`avg_abs=${avg} peak=${peak}`;$('micBar').style.width=Math.min(100,Math.round(peak/600))+'%'}
function startMicTest(){if(micTimer)return;pollMic();micTimer=setInterval(pollMic,350)}
function stopMicTest(){if(!micTimer)return;clearInterval(micTimer);micTimer=null}
function radarStateText(s){return {0:tr('radar0'),1:tr('radar1'),2:tr('radar2'),3:tr('radar3')}[s]||tr('unknown')}
async function pollRadar(){let t=await diag('radar target');let has=val(t,'has_target'),state=val(t,'state'),md=val(t,'moving_distance_cm'),me=val(t,'moving_energy'),sd=val(t,'static_distance_cm'),se=val(t,'static_energy');$('radarOccupied').textContent=has==='1'?tr('present'):(has==='0'?tr('absent'):tr('unknown'));$('radarDistance').textContent=tr('radarUnused');$('radarRaw').textContent=tr('movingRaw')+' '+(md||'-')+' cm / '+tr('energy')+' '+(me||'-')+'; '+tr('staticRaw')+' '+(sd||'-')+' cm / '+tr('energy')+' '+(se||'-');$('radarState').textContent=radarStateText(state)}
function startRadarTest(){if(radarTimer)return;pollRadar();radarTimer=setInterval(pollRadar,700)}
function stopRadarTest(){if(!radarTimer)return;clearInterval(radarTimer);radarTimer=null}
async function mcpTool(n,a){$('diag').textContent='执行中：'+n;let p={jsonrpc:'2.0',id:Date.now(),method:'tools/call',params:{name:n,arguments:a||{}}};let r=await fetch('/api/mcp',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(p)});$('diag').textContent=await r.text()}
loadStatus();setInterval(loadStatus,5000);
</script></main></body></html>)HTML");
    sendText(200, "text/html; charset=utf-8", body);
    return;
  }
  const String ip = state_ == NetworkState::Connected ? WiFi.localIP().toString()
                                                       : WiFi.softAPIP().toString();
  String body;
  body.reserve(11800);
  body += F("<!doctype html><html><head><meta charset='utf-8'>");
  body += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>铜豆控制台</title><style>");
  body += F("body{font-family:Arial,'Microsoft YaHei',sans-serif;margin:0;background:#121212;color:#f4f1ea}");
  body += F("main{max-width:920px;margin:auto;padding:22px}h1{margin:0 0 6px}.sub{color:#aaa;margin:0 0 18px}");
  body += F("input,button{box-sizing:border-box;width:100%;padding:12px;margin:6px 0;border-radius:8px}input{background:#0f0f0f;color:#f4f1ea;border:1px solid #2a2a2a}");
  body += F("button{border:0;background:#e7b45f;color:#111;font-weight:700;cursor:pointer}button:disabled{background:#555;color:#aaa;cursor:not-allowed}");
  body += F(".grid,.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px}.cards{grid-template-columns:repeat(auto-fit,minmax(190px,1fr))}.status{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:8px}");
  body += F(".card{background:#1d1d1d;padding:18px;border-radius:10px;margin:14px 0}.item,.pcard{background:#0f0f0f;border:1px solid #2a2a2a;border-radius:8px;padding:12px}.pcard{min-height:132px;text-align:left;color:#f4f1ea}.pcard.active{border-color:#e7b45f;background:rgba(231,180,95,.16)}");
  body += F(".pcard span,.pcard strong,.pcard small{display:block}.pcard span{font-size:24px}.pcard strong{font-size:17px;margin-top:8px}.label{color:#aaa;font-size:12px}.value{font-size:18px;margin-top:4px}pre{white-space:pre-wrap;background:#0b0b0b;padding:12px;border-radius:8px;min-height:100px}");
  body += F("small,.note{color:#aaa}code{color:#e7b45f}details{margin:14px 0}summary{padding:14px 16px;border-radius:8px;background:#1d1d1d;color:#e7b45f;font-weight:700;cursor:pointer}</style></head><body><main>");
  body += F("<h1>铜豆控制台</h1><p class='sub'>状态、人格和少量必要设置。硬件诊断默认收起来。</p>");
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
  body += F("<div class='card'><h2>选择铜豆今天的人格</h2><div class='cards' role='radiogroup' aria-label='铜豆人格模式'>");
  body += F("<button class='pcard' data-personality='gentle' role='radio' aria-checked='false'><span>🍵</span><strong>温柔提醒</strong><small>安静专注不打扰，适合专心搞钱的时刻。</small></button>");
  body += F("<button class='pcard active' data-personality='balanced' role='radio' aria-checked='true'><span>😜</span><strong>嘴欠搭子</strong><small>嘴硬心软，有分寸地小闹腾，陪你吐槽打工生活。</small></button>");
  body += F("<button class='pcard' data-personality='dramatic' role='radio' aria-checked='false'><span>🎭</span><strong>彻底戏精</strong><small>高频加戏，放飞自我。老板在身边时慎用。</small></button>");
  body += F("</div><small id='personaMsg'></small></div>");
  body += F("<div class='card'><h2>连接和时间</h2><form method='post' action='/api/setup/wifi'>");
  body += F("<input name='ssid' placeholder='WiFi 名称' required>");
  body += F("<input name='password' type='password' placeholder='WiFi 密码'>");
  body += F("<button type='submit'>保存并连接</button></form><button onclick='syncTime()'>用手机/电脑时间同步</button><small id='msg'></small></div>");
  body += F("<details><summary>展开硬件诊断</summary><div class='card'><h2>硬件诊断</h2><p><small>这些按钮会调用固件里的硬件自检服务。</small></p>");
  body += F("<h3>状态读取</h3><div class='grid'>");
  body += F("<button onclick=\"diag('selftest')\">完整自检</button>");
  body += F("<button onclick=\"diag('status')\">硬件状态</button>");
  body += F("<button onclick=\"diag('battery')\">电池/充电</button>");
  body += F("<button onclick=\"diag('i2c scan')\">I2C 扫描</button>");
  body += F("<button onclick=\"diag('imu')\">QMI8658A</button>");
  body += F("<button onclick=\"diag('logo touch')\">Logo 触摸</button>");
  body += F("<button onclick=\"diag('radar')\">雷达</button>");
  body += F("<button onclick=\"diag('mic')\">麦克风</button>");
  body += F("<button onclick=\"startMicTest()\">开始麦克风测试</button>");
  body += F("<button onclick=\"stopMicTest()\">停止麦克风测试</button>");
  body += F("</div><h3>输出测试</h3><div class='grid'>");
  body += F("<button onclick=\"diag('speaker')\">喇叭测试音</button>");
  body += F("<button onclick=\"diag('audio cue success')\">成功提示</button>");
  body += F("<button onclick=\"diag('audio cue error')\">错误提示</button>");
  body += F("<button onclick=\"diag('audio cue wifi en-GB')\">配网英文</button>");
  body += F("<button onclick=\"diag('audio cue wifi fr-FR')\">配网法语</button>");
  body += F("<button onclick=\"diag('audio cue wifi nl-NL')\">配网荷兰语</button>");
  body += F("<button onclick=\"diag('audio code 1863 ru-RU')\">验证码俄语</button>");
  body += F("<button onclick=\"diag('led red')\">红灯</button>");
  body += F("<button onclick=\"diag('led green')\">绿灯</button>");
  body += F("<button onclick=\"diag('led blue')\">蓝灯</button>");
  body += F("<button onclick=\"diag('led off')\">关灯</button>");
  body += F("<button onclick=\"diag('motor fault')\">电机故障</button>");
  body += F("<button onclick=\"diag('motor fault simulate on')\">模拟电机故障</button>");
  body += F("<button onclick=\"diag('motor fault simulate off')\">取消模拟故障</button>");
  body += F("<button onclick=\"diag('motor forward')\">电机正转</button>");
  body += F("<button onclick=\"diag('motor reverse')\">电机反转</button>");
  body += F("<button onclick=\"diag('motor turn left')\">电机左扭</button>");
  body += F("<button onclick=\"diag('motor turn right')\">电机右扭</button>");
  body += F("<button onclick=\"diag('motor stop')\">电机停止</button>");
  body += F("</div><h3>剧情测试</h3><div class='grid'>");
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'boot_completed'})\">开机剧情</button>");
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'voice_recognition_failed'})\">识别失败补救</button>");
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'logo_single_tapped'})\">Logo 单击</button>");
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'logo_double_tapped'})\">Logo 双击</button>");
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'logo_long_pressed'})\">Logo 长按</button>");
  body += F("<button onclick=\"mcpTool('voice_status',{})\">语音状态</button>");
  body += F("</div><pre id='diag'>等待诊断命令。</pre></div></details>");
  body += F("<div class='card'><h2>剧情配置</h2><div class='note'>剧情包编辑器后续接入。当前先用上方人格卡片和诊断区剧情测试。</div></div>");
  body += F("<script>");
  body += F("function modeText(m){return {normal:'正常',portal:'配网',connecting:'联网中',degraded:'降级'}[m]||m;}");
  body += F("function capText(c){let a=[];if(c.reminders)a.push('提醒');if(c.web)a.push('网页');if(c.visual)a.push('视觉');if(c.sound)a.push('声音');if(c.voiceInput)a.push('语音输入');return a.join(' / ')||'无';}");
  body += F("async function loadStatus(){try{let r=await fetch('/api/status');let j=await r.json();document.getElementById('wifi').textContent=j.wifiConnected?'已连接':(j.portal?'配网模式':'连接中');document.getElementById('mode').textContent=modeText(j.systemMode);document.getElementById('hardware').textContent=j.hardware.degraded?(j.hardware.motorFault?'电机故障':'降级'):'正常';document.getElementById('capability').textContent=capText(j.capabilities);}catch(e){document.getElementById('wifi').textContent='读取失败';document.getElementById('mode').textContent='读取失败';document.getElementById('hardware').textContent='读取失败';document.getElementById('capability').textContent='读取失败';}}");
  body += F("async function syncTime(){let e=Math.floor(Date.now()/1000);let r=await fetch('/api/time/sync',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'epoch='+e});document.getElementById('msg').textContent=await r.text();}");
  body += F("async function diag(c){let o=document.getElementById('diag');o.textContent='执行中：'+c;try{let r=await fetch('/api/diagnostic',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'command='+encodeURIComponent(c)});o.textContent=await r.text();}catch(e){o.textContent='诊断失败：'+e;}}");
  body += F("let micTimer=null;async function pollMic(){await diag('mic');}function startMicTest(){if(micTimer)return;pollMic();micTimer=setInterval(pollMic,300);}function stopMicTest(){if(!micTimer)return;clearInterval(micTimer);micTimer=null;}");
  body += F("async function mcpTool(n,a){let o=document.getElementById('diag');o.textContent='执行中：'+n;try{let p={jsonrpc:'2.0',id:Date.now(),method:'tools/call',params:{name:n,arguments:a||{}}};let r=await fetch('/api/mcp',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(p)});o.textContent=await r.text();}catch(e){o.textContent='MCP 调用失败：'+e;}}");
  body += F("function setP(v){document.querySelectorAll('[data-personality]').forEach(b=>{let a=b.dataset.personality===v;b.classList.toggle('active',a);b.setAttribute('aria-checked',a?'true':'false');});let m=document.getElementById('personaMsg');if(m)m.textContent='人格模式设置中';mcpTool('set_personality',{personality:v}).then(()=>{if(m)m.textContent='人格模式已发送';});}document.querySelectorAll('[data-personality]').forEach(b=>b.onclick=()=>setP(b.dataset.personality));");
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
  if (state_ == NetworkState::Portal) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false, false);
    delay(kStationDisconnectSettleMs);
  }

  WiFi.scanDelete();
  int count = WiFi.scanNetworks(false, false, false, kWifiScanFirstPassMsPerChannel);
  if (count <= 0) {
    Serial.print("wifi scan first pass count=");
    Serial.println(count);
    delay(120);
    if (state_ == NetworkState::Portal) {
      WiFi.mode(WIFI_AP_STA);
      WiFi.disconnect(false, false);
      delay(kStationDisconnectSettleMs);
    }
    WiFi.scanDelete();
    count = WiFi.scanNetworks(false, false, false, kWifiScanRetryMsPerChannel);
  }

  Serial.print("wifi scan count=");
  Serial.println(count);
  String body = "[";
  bool first = true;
  if (count > 0) {
    for (int i = 0; i < count; ++i) {
      const String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) {
        continue;
      }
      if (!first) {
        body += ",";
      }
      first = false;
      body += "{\"ssid\":\"";
      body += jsonEscape(ssid);
      body += "\",\"rssi\":";
      body += WiFi.RSSI(i);
      body += "}";
    }
  }
  body += "]";
  WiFi.scanDelete();
  sendText(200, "application/json", body);
}

void WebConfigServer::handleLocaleGet() {
  const String locale = deviceLocaleStore_.locale();
  String body;
  body.reserve(96);
  body += "{\"locale\":\"";
  body += jsonEscape(locale);
  body += "\",\"name\":\"";
  body += localeDisplayName(locale);
  body += "\"}";
  sendText(200, "application/json", body);
}

void WebConfigServer::handleLocaleSave() {
  String locale = server_.arg("locale");
  locale.trim();

  if (!DeviceLocaleStore::supported(locale)) {
    sendText(400, "text/plain; charset=utf-8", "不支持这个语言");
    return;
  }

  const LocaleResourceUpdateResult updateResult =
      localeResourceUpdateService_.downloadAndInstall(locale);
  if (!updateResult.ok) {
    sendText(503, "text/plain; charset=utf-8",
             String("语言资源更新失败：") + updateResult.error);
    return;
  }

  if (!deviceLocaleStore_.save(locale)) {
    sendText(500, "text/plain; charset=utf-8", "语言保存失败");
    return;
  }

  localeChangedEvent_ = true;
  sendText(200, "text/plain; charset=utf-8",
           String("语言资源已更新并启用：") + localeDisplayName(locale));
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

void WebConfigServer::handleVoiceDebugPage() {
  String body;
  body.reserve(5200);
  body += F(R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tong Dou Voice</title>
<style>
body{font-family:Arial,'Microsoft YaHei',sans-serif;margin:0;background:#111;color:#f5f2ea}
main{max-width:720px;margin:auto;padding:18px}
h1{font-size:24px;margin:0 0 14px}
section{border-top:1px solid #333;padding:14px 0}
label{display:block;color:#b9b2a3;font-size:13px;margin-top:10px}
input,button,textarea{box-sizing:border-box;width:100%;padding:12px;border-radius:8px;margin-top:6px}
input,textarea{background:#0b0b0b;border:1px solid #333;color:#f5f2ea}
button{border:0;background:#e1b35f;color:#111;font-weight:700}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}
pre{white-space:pre-wrap;background:#050505;border:1px solid #333;border-radius:8px;padding:12px;min-height:140px}
@media(max-width:560px){.row{grid-template-columns:1fr}}
</style></head><body><main>
<h1>Tong Dou Voice</h1>
<section>
<label>Host</label><input id="host" placeholder="192.168.1.10">
<div class="row"><div><label>Port</label><input id="port" value="8100"></div>
<div><label>Path</label><input id="path" value="/xiaozhi/v1/"></div></div>
<label>Device Id</label><input id="deviceId" placeholder="empty = device mac">
<label>Client Id</label><input id="clientId" value="tongdou-demo">
<label>Token</label><input id="token" placeholder="optional">
<div class="row"><button onclick="configureBackend()">Configure</button><button onclick="connectBackend()">Connect</button></div>
</section>
<section>
<label>Detect Text</label><textarea id="detectText" rows="3">你好，简单介绍一下你自己</textarea>
<button onclick="sendDetect()">Send Detect</button>
</section>
<section>
<label>Capture Ms</label><input id="captureMs" value="2500">
<div class="row"><button onclick="startVoice()">Start Voice Turn</button><button onclick="abortVoice()">Abort</button></div>
</section>
<section>
<button onclick="codecSelfTest()">Codec Self Test</button>
</section>
<section>
<div class="row"><button onclick="loadStatus()">Refresh</button><button onclick="clearOutput()">Clear</button></div>
<pre id="output">Ready</pre>
</section>
<script>
const $=id=>document.getElementById(id);
function formBody(data){return Object.keys(data).map(k=>encodeURIComponent(k)+'='+encodeURIComponent(data[k]||'')).join('&')}
async function post(url,data){let r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:formBody(data||{})});return await r.text()}
function show(v){try{$('output').textContent=JSON.stringify(JSON.parse(v),null,2)}catch(e){$('output').textContent=v}}
async function configureBackend(){show(await post('/api/voice/backend',{host:$('host').value,port:$('port').value,path:$('path').value,deviceId:$('deviceId').value,clientId:$('clientId').value,token:$('token').value}))}
async function connectBackend(){show(await post('/api/voice/connect',{}));setTimeout(loadStatus,700)}
async function sendDetect(){show(await post('/api/voice/detect',{text:$('detectText').value}));setTimeout(loadStatus,1200)}
async function startVoice(){show(await post('/api/voice/start',{captureMs:$('captureMs').value}));setTimeout(loadStatus,1200)}
async function abortVoice(){show(await post('/api/voice/abort',{}));setTimeout(loadStatus,500)}
async function codecSelfTest(){show(await post('/api/voice/codec-self-test',{}))}
async function loadStatus(){let r=await fetch('/api/voice/status');show(await r.text())}
function clearOutput(){$('output').textContent=''}
loadStatus();
</script></main></body></html>)HTML");
  sendText(200, "text/html; charset=utf-8", body);
}

void WebConfigServer::handleVoiceStatus() {
  sendText(200, "application/json", callMcpTool("voice_status", "{}"));
}

void WebConfigServer::handleVoiceBackendConfigure() {
  String host = server_.arg("host");
  host.trim();
  if (host.length() == 0) {
    sendText(400, "application/json", "{\"ok\":false,\"code\":\"missing_host\"}");
    return;
  }

  const int port = server_.arg("port").toInt() > 0 ? server_.arg("port").toInt() : 8100;
  String path = server_.arg("path");
  path.trim();
  if (path.length() == 0) {
    path = "/xiaozhi/v1/";
  }

  String arguments;
  arguments.reserve(240);
  arguments += "{\"host\":\"";
  arguments += jsonEscape(host);
  arguments += "\",\"port\":";
  arguments += port;
  arguments += ",\"path\":\"";
  arguments += jsonEscape(path);
  arguments += "\"";

  const String token = server_.arg("token");
  if (token.length() > 0) {
    arguments += ",\"token\":\"";
    arguments += jsonEscape(token);
    arguments += "\"";
  }
  const String deviceId = server_.arg("deviceId");
  if (deviceId.length() > 0) {
    arguments += ",\"deviceId\":\"";
    arguments += jsonEscape(deviceId);
    arguments += "\"";
  }
  const String clientId = server_.arg("clientId");
  if (clientId.length() > 0) {
    arguments += ",\"clientId\":\"";
    arguments += jsonEscape(clientId);
    arguments += "\"";
  }
  const String useTls = server_.arg("useTls");
  if (useTls == "1" || useTls == "true") {
    arguments += ",\"useTls\":1";
  }
  arguments += "}";

  sendText(200, "application/json", callMcpTool("configure_voice_backend", arguments));
}

void WebConfigServer::handleVoiceBackendConnect() {
  sendText(200, "application/json", callMcpTool("connect_voice_backend", "{}"));
}

void WebConfigServer::handleVoiceDetect() {
  String text = server_.arg("text");
  text.trim();
  if (text.length() == 0) {
    sendText(400, "application/json", "{\"ok\":false,\"code\":\"missing_text\"}");
    return;
  }

  String arguments;
  arguments.reserve(text.length() + 24);
  arguments += "{\"text\":\"";
  arguments += jsonEscape(text);
  arguments += "\"}";
  sendText(200, "application/json", callMcpTool("voice_detect", arguments));
}

void WebConfigServer::handleVoiceStart() {
  const int captureMs = server_.arg("captureMs").toInt() > 0
                            ? server_.arg("captureMs").toInt()
                            : 2500;
  String arguments;
  arguments.reserve(32);
  arguments += "{\"captureMs\":";
  arguments += captureMs;
  arguments += "}";
  sendText(200, "application/json", callMcpTool("start_voice_turn", arguments));
}

void WebConfigServer::handleVoiceStartAuto() {
  sendText(200, "application/json", callMcpTool("tongdou.voice.start_auto", "{}"));
}

void WebConfigServer::handleVoiceAbort() {
  sendText(200, "application/json", callMcpTool("voice_abort", "{}"));
}

void WebConfigServer::handleVoiceExit() {
  sendText(200, "application/json", callMcpTool("voice_exit", "{}"));
}

void WebConfigServer::handleVoiceCodecSelfTest() {
  sendText(200, "application/json", callMcpTool("voice_codec_self_test", "{}"));
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

String WebConfigServer::callMcpTool(const char* name, const String& argumentsJson) {
  String payload;
  payload.reserve(argumentsJson.length() + 120);
  payload += "{\"jsonrpc\":\"2.0\",\"id\":";
  payload += millis();
  payload += ",\"method\":\"tools/call\",\"params\":{\"name\":\"";
  payload += name;
  payload += "\",\"arguments\":";
  payload += argumentsJson;
  payload += "}}";
  return mcpServer_.handlePayloadText(payload);
}

String WebConfigServer::statusJson() const {
  const HardwareDiagnosticStatus hardware = hardwareSelfTest_.status();
  const BatterySnapshot battery = hardwareSelfTest_.batterySnapshot();
  const String locale = deviceLocaleStore_.locale();
  String body;
  body.reserve(1220);
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
  body += ",\"ip\":\"";
  body += state_ == NetworkState::Connected ? WiFi.localIP().toString()
                                            : WiFi.softAPIP().toString();
  body += "\"";
  body += ",\"timeReady\":";
  body += timeService_.ready() ? "true" : "false";
  body += ",\"now\":\"";
  body += timeService_.ready() ? timeService_.isoNow() : "";
  body += "\",\"systemMode\":\"";
  body += systemMode;
  body += "\",\"locale\":\"";
  body += jsonEscape(locale);
  body += "\",\"localeName\":\"";
  body += localeDisplayName(locale);
  body += "\",\"hardware\":{\"face\":";
  body += hardware.faceReady ? "true" : "false";
  body += ",\"imu\":";
  body += hardware.imuReady ? "true" : "false";
  body += ",\"logoTouch\":";
  body += hardware.logoTouchReady ? "true" : "false";
  body += ",\"led\":";
  body += hardware.ledReady ? "true" : "false";
  body += ",\"audioInput\":";
  body += hardware.audioInputReady ? "true" : "false";
  body += ",\"audioOutput\":";
  body += hardware.audioOutputReady ? "true" : "false";
  body += ",\"motorFault\":";
  body += hardware.motorFaultActive ? "true" : "false";
  body += ",\"motorFaultSimulated\":";
  body += hardware.motorFaultSimulated ? "true" : "false";
  body += ",\"motorAvailable\":";
  body += hardware.motorAvailable ? "true" : "false";
  body += ",\"degraded\":";
  body += hardware.degraded ? "true" : "false";
  body += "},\"battery\":{\"usb\":";
  body += battery.usbPresent ? "true" : "false";
  body += ",\"charging\":";
  body += battery.charging ? "true" : "false";
  body += ",\"standby\":";
  body += battery.standby ? "true" : "false";
  body += ",\"rawAdc\":";
  body += battery.rawAdc;
  body += "},\"memory\":{\"internalFree\":";
  body += ESP.getFreeHeap();
  body += ",\"internalMinFree\":";
  body += ESP.getMinFreeHeap();
  body += ",\"internalLargest\":";
  body += ESP.getMaxAllocHeap();
  body += ",\"psramTotal\":";
  body += ESP.getPsramSize();
  body += ",\"psramFree\":";
  body += ESP.getFreePsram();
  body += ",\"psramLargest\":";
  body += ESP.getMaxAllocPsram();
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
