# 铜豆网页控制台

这个目录保存铜豆网页控制台的静态原型。

当前固件首页仍由 `WebConfigServer` 内联生成，本目录暂不参与固件编译。

目标不是做普通参数页，而是逐步形成“硬件诊断 + 剧情包编辑器”：

- 查看联网和时间状态。
- 执行硬件诊断命令。
- 通过 MCP 人格卡片选择铜豆性格。
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
| `POST /api/mcp` | 转发 MCP JSON-RPC 请求 |

这些路径对应固件里的 `web/WebApiContract.h`。

`GET /api/status` 会返回 `systemMode`、`hardware` 和 `capabilities`，用于显示启动模式、硬件降级状态和当前保留能力。

`POST /api/mcp` 当前支持 `get_status`、`create_reminder`、`list_reminders`、`delete_reminder`、`play_scenario`、`set_personality`、`start_voice_turn`、`voice_status`、`play_voice_response` 和 `fail_voice_turn`。网页或调试工具只负责发送 MCP 请求，具体剧情触发、人格切换和实时语音回合交给固件里的应用命令服务。

静态原型里的剧情测试和人格选择已经走 `/api/mcp`，不直接碰硬件。

日常界面顺序是：当前状态、人格卡片、连接和时间。硬件诊断默认折叠，只有排查问题时展开。
