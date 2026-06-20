# 网页配置 API

这份文档定义铜豆网页配置页面和固件之间的接口。

当前阶段这些接口是**合同**，不是已经完成的真实 HTTP 服务。

大白话说：

> 先把插座规格定好，后面网页和固件就不会各接各的线。

---

## 1. 当前状态

已完成：

- 固件中已有接口路径合同：`firmware/src/web/WebApiContract.h`
- 固件中已有接口返回状态：`firmware/src/web/WebApiResult.h`
- 固件中已有剧情配置 API 骨架：`firmware/src/web/ScenarioConfigApi.*`
- 网页占位页面已展示接口清单：`firmware/web/*`

尚未完成：

- 真实 HTTP 服务。
- JSON 解析和序列化。
- Flash 持久化。
- 网页表单和 API 的真实联调。

---

## 2. 接口总表

| 方法 | 路径 | 用途 |
| --- | --- | --- |
| `GET` | `/api/health` | 查询设备是否在线 |
| `GET` | `/api/scenario/config` | 读取当前剧情配置 |
| `PUT` | `/api/scenario/config` | 保存完整剧情配置 |
| `POST` | `/api/scenario/reset` | 恢复默认剧情配置 |
| `GET` | `/api/scenario/export` | 导出剧情包 |
| `POST` | `/api/scenario/import` | 导入剧情包 |
| `GET` | `/api/scenario/options` | 读取网页下拉选项 |

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

## 12. 版本边界

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

## 13. 维护规则

- 新增接口前，先改 `firmware/src/web/WebApiContract.h`。
- 新增返回状态前，先改 `firmware/src/web/WebApiResult.h`。
- 新增网页下拉选项前，先改 `firmware/src/web/ScenarioConfigOptions.*`。
- 改接口后必须同步更新本文档和 `firmware/web/README.md`。
- 不要把 HTTP 解析、存储、剧情执行混进同一个文件。
