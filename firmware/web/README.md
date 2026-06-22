# 铜豆网页控制台

这个目录保存铜豆网页控制台的静态原型。

当前固件首页仍由 `WebConfigServer` 内联生成，本目录暂不参与固件编译。

目标不是做普通参数页，而是逐步形成“硬件诊断 + 剧情包编辑器”：

- 查看联网和时间状态。
- 执行硬件诊断命令。
- 选择铜豆性格。
- 开关搞怪彩蛋。
- 编辑触发条件。
- 选择屏幕表情、灯光、电机动作和语音台词。
- 导入和导出剧情包。

## API 规划

前端后续统一调用这些接口：

| 方法和路径 | 用途 |
| --- | --- |
| `GET /api/health` | 查询设备是否在线 |
| `GET /api/status` | 查询联网和时间状态 |
| `GET /api/scenario/config` | 读取当前剧情配置 |
| `PUT /api/scenario/config` | 保存完整剧情配置 |
| `POST /api/scenario/reset` | 恢复默认剧情配置 |
| `GET /api/scenario/export` | 导出剧情包 |
| `POST /api/scenario/import` | 导入剧情包 |
| `GET /api/scenario/options` | 读取可选表情、灯光、动作、台词和事件 |
| `POST /api/diagnostic` | 执行硬件诊断命令 |

这些路径对应固件里的 `web/WebApiContract.h`。

`GET /api/status` 会返回 `systemMode`、`hardware` 和 `capabilities`，用于显示启动模式、硬件降级状态和当前保留能力。
