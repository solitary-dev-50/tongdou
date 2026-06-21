# JiShi 迁移步骤

本文是铜豆复用 JiShi 软件资产的执行手册。

迁移时必须按本文顺序来。不要整包复制，不要边想边搬，不要把 JiShi 的业务包袱带进铜豆。

## 一句话原则

铜豆不是变成 JiShi，而是用 JiShi 跑通过的软件骨架，给铜豆新版 PCB 做板机适配。

## 迁移总规则

1. 每一步只迁移一个能力。
2. 每一步必须能单独编译。
3. 每一步必须能单独提交。
4. 每一步必须更新对应文档。
5. 每一步都要保留铜豆自己的硬件边界。
6. 不迁移摄像头、舵机、儿童教育业务、双麦声源定位。
7. 发现 JiShi 模块太大时，只拆思路和小模块，不整类硬搬。

## 迁移前检查

开始迁移前先确认：

- `docs/硬件说明.md` 已经对齐新版原理图。
- `firmware/include/tongdou/Pins.h` 已经对齐新版引脚。
- 当前固件能通过 `platformio run`。
- 当前工作区已提交，方便回退。
- 本次要迁移的 JiShi 文件已经读过，不凭记忆改。

## 阶段 1：音频底层升级

目标：把铜豆当前 `AudioInput` 和 `AudioOutput` 升级成 JiShi 最新版使用的标准 I2S 通道写法。

### 当前结论：阶段 1 暂缓

2026-06-21 已实测：铜豆当前稳定编译平台固定为 `platformio/espressif32@6.10.0`，可以通过 `platformio run`，但它不包含 `driver/i2s_std.h`，不能直接使用 JiShi 最新的标准 I2S 通道接口。

也试过切到支持 Arduino ESP32 3.x 的 `pioarduino/platform-espressif32`，能找到 `driver/i2s_std.h`，但 Windows 下工具链会出现编译器入口、C++ 标准库、`newlib`/`picolibc` 头文件互相不匹配的问题。这个方向现在不能作为铜豆主线地基。

所以阶段 1 不硬迁移。当前先保留已经能编译的旧版 I2S 驱动写法，等工具链路线明确后再升级。迁移原则不变：**不能为了追 JiShi 写法，把铜豆固件搞成不可编译。**

前置条件：

- 当前官方 PlatformIO 的 `espressif32` 平台不包含新版标准 I2S 通道头文件。
- 迁移前要切到支持 Arduino ESP32 3.x 的平台。
- `firmware/platformio.ini` 中的 `platform` 应使用支持新版 I2S 驱动的平台。

参考 JiShi 文件：

- `src/hardware/audio/MicrophoneDriver.*`
- `src/hardware/audio/SpeakerDriver.*`

铜豆对应文件：

- `firmware/src/hardware/AudioInput.*`
- `firmware/src/hardware/AudioOutput.*`
- `firmware/include/tongdou/Pins.h`

### 1.1 升级麦克风输入

要做：

- 使用标准 I2S 通道接口初始化 `INMP441`。
- 保留 `AudioInput` 单一职责：只负责采样和音量诊断。
- 输出最小诊断信息：是否就绪、采样数、峰值、平均音量。

不要做：

- 不接语音识别。
- 不接大模型。
- 不写唤醒词。
- 不把提醒逻辑塞进 `AudioInput`。

验收：

- `platformio run` 通过。
- `AudioInput` 文件职责没有扩大。
- 文档更新：`docs/系统启动时序.md`、`docs/系统架构.md`、`docs/固件文件列表.md`。

### 1.2 升级喇叭输出

要做：

- 使用标准 I2S 通道接口初始化 `NS4168`。
- 保留 `playTestTone()`。
- 保留 `playClip()` 作为后续真实语音播放入口。

不要做：

- 不在开机时自动播放测试音。
- 不把 TTS 队列提前塞进 `AudioOutput`。
- 不让网页层直接控制硬件。

验收：

- `platformio run` 通过。
- `AudioOutput` 只负责输出，不懂剧情。
- 串口能看到 I2S 输出初始化状态。

## 阶段 2：硬件自检服务

目标：做一个独立的自检服务，统一调用硬件层，给网页诊断和串口诊断使用。

建议新增：

- `firmware/src/diagnostic/HardwareSelfTestService.h`
- `firmware/src/diagnostic/HardwareSelfTestService.cpp`

参考 JiShi 文件：

- `src/config/AsyncConfigServer.*`
- `src/diagnostic/RawMicDiagnostic.*`
- `data/test_console.*`

要做：

- 麦克风短采样。
- 喇叭测试音。
- 灯光测试。
- 电机短动作测试。
- 雷达输入状态。
- 电池和充电状态。

不要做：

- 不做摄像头测试。
- 不做舵机测试。
- 不做双麦声源定位。
- 不保存大体积 WAV，第一版只返回统计值。

验收：

- 自检服务不依赖网页。
- 网页和串口都可以复用它。
- 自检失败不会拖死主循环。

## 阶段 3：网页诊断控制台

目标：从 JiShi 迁移网页诊断控制台结构，改成铜豆硬件。

参考 JiShi 文件：

- `data/test_console.html`
- `data/test_console.css`
- `data/test_console.js`
- `src/config/AsyncConfigServer.*`

铜豆需要保留的页面能力：

- 当前状态。
- 麦克风采样。
- 喇叭测试音。
- 灯光测试。
- 电机测试。
- 雷达状态。
- 电池/充电状态。
- 剧情触发测试。

必须删除或改掉：

- 舵机相关文字和接口。
- 摄像头相关文字和接口。
- 双麦声源定位相关文字和接口。
- 儿童教育相关配置。

验收：

- 页面第一眼就是铜豆，不是 JiShi。
- 所有按钮都有真实后端接口或明确禁用状态。
- 网页层不直接操作硬件，必须通过自检服务或应用服务。

## 阶段 4：启动编排迁移

目标：引入 JiShi 的启动模式思想，让铜豆启动更像产品。

参考 JiShi 文件：

- `src/business/SystemBootstrap.*`
- `src/hardware/HardwareManager.*`

建议新增或改造：

- `firmware/src/system/SystemBootstrap.*`
- `firmware/src/hardware/HardwareManager.*`

启动模式：

- 配网模式：没有 WiFi 时启动热点和配置页。
- 正常模式：有配置时启动完整能力。
- 降级模式：某个硬件失败时保留其他能力。

铜豆分阶段启动建议：

1. 串口和配置存储。
2. 基础反馈：OLED、灯。
3. 安全硬件：电机默认停止。
4. 音频：麦克风、喇叭。
5. 感知：雷达、电池、充电。
6. 网络、时间、提醒。
7. 剧情和网页服务。

验收：

- `main.cpp` 只负责创建启动器并调用 `boot()` / `run()`。
- 启动失败有降级状态。
- 某个非核心硬件失败，不影响提醒和网页。

## 阶段 5：MCP 工具层迁移

目标：把 JiShi 已验证的 MCP 工具注册结构迁移到铜豆。

参考 JiShi 文件：

- `src/business/mcp/mcp_server.*`
- `src/business/mcp/mcp_tool_registry.*`
- `src/business/mcp/mcp_protocol_controller.*`
- `src/business/mcp/tools/mcp_system_tools.*`

铜豆第一批工具：

- `get_status`：获取设备状态。
- `create_reminder`：创建提醒。
- `list_reminders`：列出提醒。
- `delete_reminder`：删除提醒。
- `play_scenario`：触发指定剧情。
- `set_personality`：切换人格模式。

不要迁移：

- 教育工具。
- 摄像头工具。
- 儿童资料工具。
- 兴趣图谱工具。

验收：

- 工具列表能返回。
- 工具调用能返回结构化结果。
- 工具层不直接操作硬件，必须通过应用服务。

## 阶段 6：实时语音链路

目标：复用 JiShi 的实时音频上传和服务端音频回放结构，让铜豆真正接大模型。

参考 JiShi 文件：

- `src/realtime/RealtimeAudioPipeline.*`
- `src/realtime/RealtimeAudioSendQueue.*`
- `src/realtime/RealtimeAudioSender.*`
- `src/realtime/RealtimeTurnController.*`
- `src/tts/TtsPlaybackService.*`
- `src/tts/TtsAudioQueue.*`
- `src/audio/OpusEncoderService.*`

铜豆适配重点：

- 输入来自 `INMP441` 单麦。
- 输出到 `NS4168`。
- 人格和剧情由铜豆自己的剧情系统决定。
- 语音交互失败要进入“被咖啡呛到”等失败补救剧情。

不要做：

- 不照搬 JiShi 教育实时运行时。
- 不把儿童教育状态机带进来。
- 不让实时语音链路绕过铜豆剧情系统。

验收：

- 能开始一次语音回合。
- 能把麦克风音频送出去。
- 能播放服务端返回音频。
- 失败时能触发铜豆自己的失败剧情。

## 阶段 7：产品化清理

目标：让迁移后的固件仍然像铜豆，不像拼接出来的大工程。

检查项：

- 文件职责是否清楚。
- `App` 或 `SystemBootstrap` 是否过胖。
- 网页是否还有 JiShi、舵机、摄像头、儿童教育字样。
- MCP 工具是否都是铜豆自己的能力。
- 文档是否和代码一致。
- `platformio run` 是否通过。

## 每阶段提交格式

建议每个阶段一到多个小提交：

```text
feat: migrate audio input to standard i2s channel
feat: add hardware self-test service
feat: add TongDou diagnostic console
refactor: introduce TongDou system bootstrap
feat: migrate MCP tool registry
feat: add realtime voice pipeline skeleton
```

提交前必须跑：

```bash
cd firmware
platformio run
```

## 绝对禁止

- 禁止整包复制 JiShi。
- 禁止把 JiShi 的摄像头、舵机、儿童教育业务带进铜豆主线。
- 禁止为了快，把硬件控制直接写进网页层。
- 禁止为了快，把剧情写进硬件层。
- 禁止没有编译就提交。
- 禁止一次提交跨多个迁移阶段。

## 当前下一步

当前执行到阶段 3：

阶段 1 音频底层升级暂缓，原因是当前稳定工具链暂不支持 JiShi 最新版使用的标准 I2S 通道接口。

阶段 2 已新增硬件自检服务。现在继续推进网页诊断控制台，让网页按钮通过自检服务测试电池、雷达、麦克风、喇叭、灯和电机。
