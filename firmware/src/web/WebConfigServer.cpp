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
  server_.on("/voice", HTTP_GET, [this]() { handleVoiceDebugPage(); });
  server_.on("/api/voice/status", HTTP_GET, [this]() { handleVoiceStatus(); });
  server_.on("/api/voice/backend", HTTP_POST, [this]() { handleVoiceBackendConfigure(); });
  server_.on("/api/voice/connect", HTTP_POST, [this]() { handleVoiceBackendConnect(); });
  server_.on("/api/voice/detect", HTTP_POST, [this]() { handleVoiceDetect(); });
  server_.on("/api/voice/start", HTTP_POST, [this]() { handleVoiceStart(); });
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
button,input{box-sizing:border-box;width:100%;padding:12px;margin:5px 0;border-radius:8px}
button{border:0;background:#e7b45f;color:#111;font-weight:700;cursor:pointer}button.secondary{background:#2a2a2a;color:#f4f1ea}
input{background:#0f0f0f;color:#f4f1ea;border:1px solid #2a2a2a}
pre{white-space:pre-wrap;background:#0b0b0b;padding:12px;border-radius:8px;min-height:120px}
.meter{height:16px;background:#0b0b0b;border:1px solid #2a2a2a;border-radius:999px;overflow:hidden}.bar{height:100%;width:0;background:#e7b45f}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}@media(max-width:560px){.row{grid-template-columns:1fr}}
small{color:#aaa}
</style></head><body><main>
<h1>铜豆控制台</h1><p class="sub">旧 PCB 固件调试面板</p>

<section class="card"><h2>状态</h2><div class="status">
<div class="item"><div class="label">网络</div><div class="value" id="wifi">读取中</div></div>
<div class="item"><div class="label">地址</div><div class="value" id="ip">读取中</div></div>
<div class="item"><div class="label">时间</div><div class="value" id="time">读取中</div></div>
<div class="item"><div class="label">电池</div><div class="value" id="battery">读取中</div></div>
<div class="item"><div class="label">硬件</div><div class="value" id="hardware">读取中</div></div>
<div class="item"><div class="label">能力</div><div class="value" id="capability">读取中</div></div>
</div><button onclick="loadStatus()">刷新状态</button></section>

<section class="card"><h2>连接和时间</h2>
<form method="post" action="/api/setup/wifi">
<input name="ssid" placeholder="WiFi 名称" required>
<input name="password" type="password" placeholder="WiFi 密码">
<button type="submit">保存并连接</button>
</form><button onclick="syncTime()">同步当前时间</button><small id="msg"></small></section>

<section class="card"><h2>麦克风</h2>
<div class="meter"><div class="bar" id="micBar"></div></div>
<div class="row"><button onclick="startMicTest()">开始持续测试</button><button class="secondary" onclick="stopMicTest()">停止</button></div>
<div class="item"><div class="label">当前读数</div><div class="value" id="micValue">未开始</div></div></section>

<section class="card"><h2>雷达</h2>
<div class="status">
<div class="item"><div class="label">是否有人</div><div class="value" id="radarOccupied">未读取</div></div>
<div class="item"><div class="label">距离</div><div class="value" id="radarDistance">不用于判断</div></div>
<div class="item"><div class="label">原始辅助值</div><div class="value" id="radarRaw">未读取</div></div>
<div class="item"><div class="label">状态</div><div class="value" id="radarState">未读取</div></div>
</div>
<div class="row"><button onclick="startRadarTest()">开始持续测试</button><button class="secondary" onclick="stopRadarTest()">停止</button></div></section>

<section class="card"><h2>输出测试</h2><div class="grid">
<button onclick="diag('speaker')">喇叭短音</button>
<button onclick="diag('led red')">红灯</button>
<button onclick="diag('led green')">绿灯</button>
<button onclick="diag('led blue')">蓝灯</button>
<button onclick="diag('led off')">关灯</button>
<button onclick="diag('motor forward')">电机正转</button>
<button onclick="diag('motor reverse')">电机反转</button>
<button onclick="diag('motor stop')">电机停止</button>
</div></section>

<section class="card"><h2>硬件读取</h2><div class="grid">
<button onclick="diag('selftest')">完整自检</button>
<button onclick="diag('status')">硬件状态</button>
<button onclick="diag('battery')">电池/充电</button>
<button onclick="diag('radar')">雷达电平</button>
<button onclick="diag('radar target')">雷达目标</button>
<button onclick="diag('radar sample')">雷达连续采样</button>
<button onclick="diag('mic')">麦克风单次</button>
</div></section>

<section class="card"><h2>剧情测试</h2><div class="grid">
<button onclick="mcpTool('play_scenario',{event:'boot_completed'})">开机场景</button>
<button onclick="mcpTool('play_scenario',{event:'voice_recognition_failed'})">识别失败</button>
<button onclick="mcpTool('voice_status',{})">语音状态</button>
</div></section>

<section class="card"><h2>输出</h2><pre id="diag">等待操作</pre></section>

<script>
const $=id=>document.getElementById(id);let micTimer=null,radarTimer=null;
function capText(c){let a=[];if(c.reminders)a.push('提醒');if(c.web)a.push('网页');if(c.visual)a.push('视觉');if(c.sound)a.push('声音');if(c.voiceInput)a.push('麦克风');return a.join(' / ')||'无'}
async function loadStatus(){try{let r=await fetch('/api/status');let j=await r.json();$('wifi').textContent=j.wifiConnected?'已连接':(j.portal?'配网模式':'连接中');$('ip').textContent=j.ip||'-';$('time').textContent=j.timeReady?j.now:'未校时';$('battery').textContent=`USB ${j.battery.usb?'有':'无'} / 充电 ${j.battery.charging?'是':'否'} / ADC ${j.battery.rawAdc}`;$('hardware').textContent=j.hardware.degraded?'降级':'正常';$('capability').textContent=capText(j.capabilities)}catch(e){$('wifi').textContent='读取失败'}}
async function syncTime(){let e=Math.floor(Date.now()/1000);let r=await fetch('/api/time/sync',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'epoch='+e});$('msg').textContent=await r.text();loadStatus()}
async function diag(c){$('diag').textContent='执行中：'+c;let r=await fetch('/api/diagnostic',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'command='+encodeURIComponent(c)});let t=await r.text();$('diag').textContent=t;return t}
function val(t,k){let m=t.match(new RegExp(k+'=([^\\n\\r]+)'));return m?m[1].trim():''}
async function pollMic(){let t=await diag('mic');let avg=Number(val(t,'avg_abs')||0),peak=Number(val(t,'peak')||0);$('micValue').textContent=`avg_abs=${avg} peak=${peak}`;$('micBar').style.width=Math.min(100,Math.round(peak/600))+'%'}
function startMicTest(){if(micTimer)return;pollMic();micTimer=setInterval(pollMic,350)}
function stopMicTest(){if(!micTimer)return;clearInterval(micTimer);micTimer=null}
function radarStateText(s){return {0:'无人',1:'移动目标',2:'静止目标',3:'移动+静止'}[s]||'未知'}
async function pollRadar(){let t=await diag('radar target');let has=val(t,'has_target'),state=val(t,'state'),md=val(t,'moving_distance_cm'),me=val(t,'moving_energy'),sd=val(t,'static_distance_cm'),se=val(t,'static_energy');$('radarOccupied').textContent=has==='1'?'有人':(has==='0'?'无人':'未知');$('radarDistance').textContent='不可用：LD2412 只用于有人/无人';$('radarRaw').textContent='移动原始 '+(md||'-')+' cm / 能量 '+(me||'-')+'；静止原始 '+(sd||'-')+' cm / 能量 '+(se||'-');$('radarState').textContent=radarStateText(state)}
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
  body += F("<button onclick=\"diag('radar')\">雷达</button>");
  body += F("<button onclick=\"diag('mic')\">麦克风</button>");
  body += F("<button onclick=\"startMicTest()\">开始麦克风测试</button>");
  body += F("<button onclick=\"stopMicTest()\">停止麦克风测试</button>");
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
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'boot_completed'})\">开机剧情</button>");
  body += F("<button onclick=\"mcpTool('play_scenario',{event:'voice_recognition_failed'})\">识别失败补救</button>");
  body += F("<button onclick=\"mcpTool('voice_status',{})\">语音状态</button>");
  body += F("</div><pre id='diag'>等待诊断命令。</pre></div></details>");
  body += F("<div class='card'><h2>剧情配置</h2><div class='note'>剧情包编辑器后续接入。当前先用上方人格卡片和诊断区剧情测试。</div></div>");
  body += F("<script>");
  body += F("function modeText(m){return {normal:'正常',portal:'配网',connecting:'联网中',degraded:'降级'}[m]||m;}");
  body += F("function capText(c){let a=[];if(c.reminders)a.push('提醒');if(c.web)a.push('网页');if(c.visual)a.push('视觉');if(c.sound)a.push('声音');if(c.voiceInput)a.push('语音输入');return a.join(' / ')||'无';}");
  body += F("async function loadStatus(){try{let r=await fetch('/api/status');let j=await r.json();document.getElementById('wifi').textContent=j.wifiConnected?'已连接':(j.portal?'配网模式':'连接中');document.getElementById('mode').textContent=modeText(j.systemMode);document.getElementById('hardware').textContent=j.hardware.degraded?'降级':'正常';document.getElementById('capability').textContent=capText(j.capabilities);}catch(e){document.getElementById('wifi').textContent='读取失败';document.getElementById('mode').textContent='读取失败';document.getElementById('hardware').textContent='读取失败';document.getElementById('capability').textContent='读取失败';}}");
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
<div class="row"><div><label>Port</label><input id="port" value="8000"></div>
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
<button onclick="startVoice()">Start Voice Turn</button>
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

  const int port = server_.arg("port").toInt() > 0 ? server_.arg("port").toInt() : 8000;
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
  String body;
  body.reserve(760);
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
  body += "},\"battery\":{\"usb\":";
  body += battery.usbPresent ? "true" : "false";
  body += ",\"charging\":";
  body += battery.charging ? "true" : "false";
  body += ",\"standby\":";
  body += battery.standby ? "true" : "false";
  body += ",\"rawAdc\":";
  body += battery.rawAdc;
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
