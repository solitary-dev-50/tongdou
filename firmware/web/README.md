# 铜豆网页配置占位

这个目录预留给后续网页配置界面。

目标不是做普通参数页，而是做“剧情包编辑器”：

- 选择铜豆性格。
- 开关搞怪彩蛋。
- 编辑触发条件。
- 选择屏幕表情、灯光、电机动作和语音台词。
- 导入和导出剧情包。

当前只占位，不参与固件编译。

## API 规划

前端后续统一调用这些接口：

| 方法和路径 | 用途 |
| --- | --- |
| `GET /api/health` | 查询设备是否在线 |
| `GET /api/scenario/config` | 读取当前剧情配置 |
| `PUT /api/scenario/config` | 保存完整剧情配置 |
| `POST /api/scenario/reset` | 恢复默认剧情配置 |
| `GET /api/scenario/export` | 导出剧情包 |
| `POST /api/scenario/import` | 导入剧情包 |
| `GET /api/scenario/options` | 读取可选表情、灯光、动作、台词和事件 |

这些路径对应固件里的 `web/WebApiContract.h`。
