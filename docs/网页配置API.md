# 网页配置 API

这份文档定义铜豆网页配置页面和固件之间的接口。

当前阶段这些接口一部分已经是真实 HTTP 服务，一部分仍是后续合同。

大白话说：

> 先把插座规格定好，后面网页和固件就不会各接各的线。

---

## 1. 当前状态

已完成：

- 固件中已有接口路径合同：`firmware/src/web/WebApiContract.h`
- 固件中已有接口返回状态：`firmware/src/web/WebApiResult.h`
- 固件中已有剧情配置 API 骨架：`firmware/src/web/ScenarioConfigApi.*`
- 固件首页已有配网、校时和硬件诊断控制台。
- 静态网页原型已同步硬件诊断界面：`firmware/web/*`
- 固件已提供 `/api/mcp`，用于把网页收到的 MCP JSON-RPC 请求转交给 `McpServer`。

尚未完成：

- 剧情配置页面的完整表单。
- 剧情配置 JSON 的完整读写联调。
- 独立的剧情触发 HTTP 接口；当前剧情测试先通过 `/api/mcp` 调用 `play_scenario`。

---

## 2. 接口总表

| 方法 | 路径 | 用途 |
| --- | --- | --- |
| `GET` | `/api/health` | 查询设备是否在线 |
| `GET` | `/api/status` | 查询联网和时间状态 |
| `GET` | `/api/wifi/scan` | 扫描附近 WiFi |
| `POST` | `/api/setup/wifi` | 保存 WiFi 并开始连接 |
| `GET` | `/api/locale` | 读取设备语言 |
| `POST` | `/api/locale` | 下载、校验并启用设备语言资源 |
| `GET` | `/api/reminders` | 查询设备真实保存的提醒 |
| `POST` | `/api/reminders` | 创建一次性提醒，可附带预渲染正文位图 |
| `POST` | `/api/reminders/delete` | 删除提醒 |
| `GET` | `/api/scenario/config` | 读取当前剧情配置 |
| `PUT` | `/api/scenario/config` | 保存完整剧情配置 |
| `POST` | `/api/scenario/reset` | 恢复默认剧情配置 |
| `GET` | `/api/scenario/export` | 导出剧情包 |
| `POST` | `/api/scenario/import` | 导入剧情包 |
| `GET` | `/api/scenario/options` | 读取网页下拉选项 |
| `POST` | `/api/diagnostic` | 执行硬件诊断命令 |
| `POST` | `/api/mcp` | 转发 MCP JSON-RPC 请求 |

---

## 3. 通用返回格式

建议后续统一返回：

```json
{
  "ok": true,
  "error": null,
  "data": {}
}
```

失败时：

```json
{
  "ok": false,
  "error": {
    "code": "invalid_request",
    "message": "请求内容无效"
  },
  "data": null
}
```

---

## 4. 错误码

对应 `firmware/src/web/WebApiResult.h`。

| 错误码 | 含义 |
| --- | --- |
| `ok` | 成功 |
| `invalid_request` | 请求内容无效 |
| `storage_error` | 配置读取或保存失败 |
| `not_implemented` | 接口尚未实现 |

---

## 5. GET /api/health

用途：

> 查询设备是否在线，后续网页打开时先调用它。

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "device": "tongdou",
    "status": "ready",
    "version": "dev"
  }
}
```

状态说明：

| 状态 | 含义 |
| --- | --- |
| `ready` | 设备正常 |
| `degraded` | 设备部分能力降级 |
| `booting` | 设备启动中 |

---

## 5.1 GET /api/status

用途：

> 查询联网、时间、启动模式和硬件状态。

当前返回：

```json
{
  "wifiConnected": true,
  "portal": false,
  "timeReady": true,
  "now": "2026-06-22 10:00:00",
  "systemMode": "normal",
  "locale": "en-GB",
  "localeName": "English UK",
  "hardware": {
    "face": true,
    "led": true,
    "audioInput": true,
    "audioOutput": true,
    "degraded": false
  },
  "capabilities": {
    "reminders": true,
    "web": true,
    "visual": true,
    "sound": true,
    "voiceInput": true
  }
}
```

`systemMode` 说明：

| 状态 | 含义 |
| --- | --- |
| `normal` | 正常模式 |
| `portal` | 配网模式 |
| `connecting` | 正在联网 |
| `degraded` | 硬件降级 |

---

## 5.2 GET /api/locale

用途：

> 读取设备当前语言，用于配置页下拉框、提示音和后续语音服务默认语言。

当前返回：

```json
{
  "locale": "en-GB",
  "name": "English UK"
}
```

---

## 5.3 POST /api/locale

用途：

> 从当前语音服务器下载对应语言资源包，校验并覆盖唯一资源分区，成功后才保存设备语言。

请求体：

```text
locale=en-GB
```

当前支持：

- `English UK`
- `English Canada`
- `Français`
- `Français Canada`
- `Nederlands`
- `Русский`
- `中文`

执行顺序：

```text
检查语言是否支持
  -> 从 WebSocket hello 取得服务器明确下发的资源地址
  -> 下载完整资源包到外部扩展内存
  -> 校验格式、语言、边界、必需文件和 SHA-256 摘要
  -> 覆盖 locale 资源分区
  -> 从闪存重新读取并校验
  -> 最后保存设备语言
  -> 播放新语言的配网提示
```

返回：

- `200`：资源更新、闪存复核和语言保存全部成功。
- `400`：语言不受支持。
- `503`：服务器地址缺失、网络失败、资源包下载或校验失败。
- `500`：资源已经写入，但设备语言保存失败。

网页只有收到 `200` 才切换页面语言；失败时下拉框恢复原值。服务器下载失败或下载包校验失败时，不覆盖原资源，也不保存新语言。

若在擦写闪存期间发生硬件写入故障或掉电，启动校验会拒绝损坏资源，原语言设置仍保留。服务器下一次 WebSocket `hello` 完成且语音状态空闲后，固件会自动重新下载当前语言；失败后至少等待 60 秒再重试。修复完成前，欢迎、配网等既有提示暂时使用旧内嵌语音，通用离线播报不可用时只播放实时生成的三声提示音。

---

## 5.4 GET /api/wifi/scan

用途：

> 扫描附近 WiFi，给配网页面下拉和按钮列表使用。

当前行为：

- 扫描前清掉旧结果。
- 扫不到时自动重试一次。
- 返回时跳过空名字的隐藏网络。

当前返回：

```json
[
  {
    "ssid": "Home WiFi",
    "rssi": -45
  }
]
```

如果仍然扫不到，返回空数组，页面继续允许用户手动填写。

---

## 5.5 POST /api/setup/wifi

用途：

> 保存 WiFi 名称和密码，并让设备开始连接。

请求体：

```text
ssid=Home%20WiFi&password=12345678
```

返回：纯文本保存结果。

---

## 5.6 提醒接口

### GET /api/reminders

返回设备真实保存的提醒列表：

```json
[
  {
    "id": 5,
    "dueAt": 1787618937,
    "completed": false,
    "displayBitmap": true,
    "text": "喝水，站起来活动一下"
  }
]
```

`displayBitmap` 只表示该提醒已经保存 128×48 单色正文位图，不返回位图正文。

### POST /api/reminders

请求体使用表单参数：

```text
dueAt=1787618937&text=喝水，站起来活动一下&displayBitmapBase64=...
```

- `dueAt`：必填，Unix 秒级时间戳，设备时间已同步时不能早于当前时间。
- `text`：必填，提醒正文。
- `displayBitmapBase64`：可选；解码后必须正好是 768 字节，格式为 SSD1306 的 128×48、1 位分页位图。

普通语音提醒不需要网页自己生成位图：服务器在调用设备 MCP 创建工具前会自动渲染并注入。直接调用本网页接口时，如果希望任意语言正文准确上屏，调用方必须提供位图；不提供仍可创建提醒，但到期时没有任意文字上屏能力。

成功返回：

```json
{
  "ok": true,
  "id": 5,
  "dueAt": 1787618937,
  "displayBitmap": true
}
```

无效位图返回 `400`，提醒存储已满返回 `507`。

### POST /api/reminders/delete

请求体：

```text
id=5
```

成功返回 `{"ok":true}`，提醒不存在返回 `404`。

---

## 6. GET /api/scenario/config

用途：

> 读取当前剧情配置。

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "defaultPersonality": "balanced",
    "enableCustomScenarios": false,
    "enableDramaticEasterEggs": false,
    "rules": []
  }
}
```

---

## 7. PUT /api/scenario/config

用途：

> 保存完整剧情配置。

请求体：

```json
{
  "defaultPersonality": "balanced",
  "enableCustomScenarios": true,
  "enableDramaticEasterEggs": false,
  "rules": [
    {
      "enabled": true,
      "eventType": "overtime_reminder_due",
      "personality": "balanced",
      "face": "squint",
      "light": "dim_warm",
      "motion": "lean_forward",
      "voice": "overtime_nudge",
      "durationMs": 1800
    }
  ]
}
```

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "saved": true
  }
}
```

边界：

- 保存的是完整配置，不做局部字段 PATCH。
- 第一版最多保存 `16` 条用户剧情规则。
- 保存失败时必须返回 `storage_error`。

---

## 8. POST /api/scenario/reset

用途：

> 恢复默认剧情配置。

请求体：

```json
{}
```

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "reset": true
  }
}
```

---

## 9. GET /api/scenario/export

用途：

> 导出当前剧情包，方便用户备份或分享。

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "format": "tongdou-scenario-pack",
    "version": 1,
    "config": {
      "defaultPersonality": "balanced",
      "enableCustomScenarios": false,
      "enableDramaticEasterEggs": false,
      "rules": []
    }
  }
}
```

---

## 10. POST /api/scenario/import

用途：

> 导入剧情包。

请求体：

```json
{
  "format": "tongdou-scenario-pack",
  "version": 1,
  "config": {
    "defaultPersonality": "balanced",
    "enableCustomScenarios": false,
    "enableDramaticEasterEggs": false,
    "rules": []
  }
}
```

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "imported": true
  }
}
```

边界：

- 导入前必须校验格式和版本。
- 不认识的格式返回 `invalid_request`。
- 未来版本升级时再做兼容转换。

---

## 11. GET /api/scenario/options

用途：

> 给网页配置页面提供下拉选项。

建议返回：

```json
{
  "ok": true,
  "error": null,
  "data": {
    "personalities": ["gentle", "balanced", "dramatic"],
    "events": [
      "boot_completed",
      "user_arrived",
      "user_left",
      "user_idle_too_long",
      "reminder_due",
      "reminder_confirmed",
      "reminder_snoozed",
      "quiet_mode_requested",
      "low_battery",
      "charging_started",
      "overtime_reminder_due"
    ],
    "faces": [
      "none",
      "wake_up",
      "smile",
      "serious",
      "roll_eyes",
      "sleepy",
      "wronged",
      "squint",
      "innocent"
    ],
    "lights": [
      "none",
      "off",
      "soft_white",
      "warm_wake",
      "red_short_blink",
      "weak_breath",
      "dim_warm"
    ],
    "motions": [
      "none",
      "stop",
      "nod",
      "tiny_shake",
      "nudge_forward",
      "lean_forward",
      "shrink_back"
    ],
    "voices": [
      "none",
      "wake_greeting",
      "snooze_soft",
      "snooze_tease",
      "confirm_reward",
      "quiet_reply",
      "break_reminder",
      "overtime_nudge",
      "overtime_coward_reply"
    ]
  }
}
```

---

## 12. POST /api/diagnostic

用途：

> 给网页诊断控制台调用硬件自检服务。

请求体：

```text
command=selftest
```

当前支持命令：

- `selftest`
- `status`
- `battery`
- `radar`
- `mic`
- `speaker`
- `led red`
- `led green`
- `led blue`
- `led off`
- `motor forward`
- `motor reverse`
- `motor stop`

返回：纯文本诊断结果。

边界：

- 网页层不直接操作硬件。
- 命令必须交给 `HardwareSelfTestService` 执行。
- 电机测试必须是短动作或可停止动作。

---

## 12.1 POST /api/mcp

用途：

> 给电脑端大模型或调试工具调用铜豆 MCP 工具。

请求体：标准 JSON-RPC 文本。

示例：

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/list",
  "params": {}
}
```

当前工具：

- `get_status`
- `create_reminder`
- `list_reminders`
- `delete_reminder`
- `play_scenario`
- `set_personality`
- `start_voice_turn`
- `voice_status`
- `tongdou.voice.start_auto`
- `tongdou.voice.abort`
- `tongdou.voice.exit`
- `play_voice_response`
- `fail_voice_turn`
- `tongdou.audio.get_volume`
- `tongdou.audio.set_volume`

查询当前喇叭音量：

```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/call",
  "params": {
    "name": "tongdou.audio.get_volume",
    "arguments": {}
  }
}
```

成功结果中的 `volume` 是设备当前真实音量，范围 `0` 到 `100`；`muted` 表示当前是否为静音。查询不会修改音量。

设置喇叭音量：

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "tongdou.audio.set_volume",
    "arguments": {
      "volume": 50
    }
  }
}
```

`volume` 表示绝对音量，范围 `0` 到 `100`；`delta` 表示相对增减。两者必须二选一，不能同时传，也不能都不传。设置会立即作用于服务器回复、本地提示音和测试音，并在断电后保留。

`voice_status` 和 `GET /api/voice/status` 的上行诊断重点字段：

- `backend.uplinkPcmFramesEncoded`：已经完成 Opus 编码的帧数。
- `backend.uplinkOpusPacketsSent`：已经成功写入网络的音频包数。
- `backend.uplinkStateRejects`、`uplinkSocketDisconnects`、`uplinkTransportFailures`：分别表示客户端状态不允许发送、底层连接已断、网络写入失败。
- `backend.uplinkSlowSends`、`lastUplinkSendElapsedMs`、`maxUplinkSendElapsedMs`：网络写入慢次数和耗时。
- `capture.maxUploadSamplesQueued`、`uploadSamplesDropped`：当前回合上传队列峰值和丢失采样数。
- `capture.uploadBufferCapacitySamples`、`uploadBufferPsramBytes`：PSRAM 上传缓冲容量，当前正式值分别为 96000 个采样点和 192000 字节。

边界：

- `/api/mcp` 只转发请求，不直接处理具体工具。
- `McpServer` 只处理 MCP 协议和工具注册。
- 剧情触发和人格切换必须交给 `AppCommandService`。
- 实时语音回合工具必须通过 `AppCommandService` 和 `RealtimeVoiceService`，不能在 MCP 层直接操作麦克风或喇叭。
- 查询和设置音量必须通过 `AppCommandService` 和 `AudioVolumeService`，不能在 MCP 层复制音量状态或直接改音频采样。

---

## 13. 版本边界

第一版只保证：

- 配置可读。
- 配置可写。
- 默认配置可恢复。
- 剧情包可导入导出。
- 网页能拿到可选项。

第一版不承诺：

- 用户账号。
- 云端同步。
- 手机 App。
- 复杂权限系统。
- 多设备联动。

---

## 14. 维护规则

- 新增接口前，先改 `firmware/src/web/WebApiContract.h`。
- 新增返回状态前，先改 `firmware/src/web/WebApiResult.h`。
- 新增网页下拉选项前，先改 `firmware/src/web/ScenarioConfigOptions.*`。
- 改接口后必须同步更新本文档和 `firmware/web/README.md`。
- 不要把 HTTP 解析、存储、剧情执行混进同一个文件。
