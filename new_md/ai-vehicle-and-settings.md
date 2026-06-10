# AI 扩展：飞机控制 + QGC 设置读写

**日期**：2026-06-10
**分支**：master

## 功能概述

在已有 AI 对话功能的基础上，给 AI 助手扩展了两类能力，均通过 OpenAI function calling（tool use）实现，AI 自主决定调用哪个工具，C++ 执行后回传结果：

1. **读取/控制无人机**：读飞行状态、执行起飞/降落/返航/飞到某点等指令
2. **读写 QGC 应用设置**：读取并修改白名单内的应用设置（音量、主题、单位、地图源等）

控制类与设置写入类操作可由独立开关启用，并支持执行前确认弹窗。

## 一、飞机控制（AIVehicleTools）

新增 `qgroundcontrol/src/Utilities/Diagnostics/AIVehicleTools.{h,cc}`

- 普通 QObject，由 AIChatService 持有；每次从 `MultiVehicleManager::instance()->activeVehicle()` 取当前活动飞机
- `toolDefinitions(includeControl)` 输出 OpenAI tools JSON；`includeControl=false` 时仅返回只读工具
- 读取类（无需确认）：
  - `get_vehicle_status`：连接状态、armed、飞行模式+可用模式、flying/landing、高度(相对/海拔)、地速/空速、爬升率、航向/姿态、距Home、油门、GPS(卫星/hdop/lock)、当前坐标、Home坐标、电池(电压/电流/剩余%/温度)、飞机类型、readyToFly
- 控制类（受确认开关约束）：
  - `takeoff(altitude_m)` / `land` / `return_to_launch` / `goto_location(lat,lon)` / `change_altitude(delta_m)` / `set_flight_mode(mode)`（校验合法模式）/ `arm` / `disarm` / `pause`
- 每个工具先检查 activeVehicle 存在，返回结构化 JSON 结果
- 取值通过 FactGroup：`vehicle->vehicleFactGroup()->getFact("altitudeRelative")->rawValue()` 等
- 未向 AI 暴露 emergencyStop / reboot 等高风险指令

## 二、QGC 设置读写（AISettingsTools）

新增 `qgroundcontrol/src/Utilities/Diagnostics/AISettingsTools.{h,cc}`

- 硬编码白名单 `{组名 → 允许的设置项}`，AI 无法操作未列出的设置
- 白名单内容：
  - `appSettings`：audioMuted, audioVolume, uiScalePercent, indoorPalette, batteryPercentRemainingAnnounce, defaultMissionItemAltitude, useChecklist, enforceChecklist
  - `unitsSettings`：horizontal/vertical/area/speed/temperature/weight Units
  - `flyViewSettings`：keepMapCenteredOnVehicle, showSimpleCameraControl, showObstacleDistanceOverlay, guidedMinimum/MaximumAltitude, maxGoToLocationDistance
  - `flightMapSettings`：mapProvider, mapType
  - `videoSettings`：gridLines, videoFit, aspectRatio
  - **明确排除** 通信/链路/固件/安全相关：autoConnect、mavlink、rtk、ntrip、firmwareUpgrade、remoteID、joystick、video 的 url/source 等
- 三个工具：
  - `list_settings(group?)`：列出白名单设置，含当前值/描述/单位/可选枚举值
  - `get_setting(group, name)`：读单个设置
  - `set_setting(group, name, value)`：改设置——白名单校验 + `Fact::validate()` 类型校验 + 枚举值按字符串或索引设置
- 通过 Qt 属性系统访问：`SettingsManager::instance()->property("appSettings").value<QObject*>()` → `group->property("audioVolume").value<Fact*>()`

## 三、AIChatService 改造

`qgroundcontrol/src/Utilities/Diagnostics/AIChatService.{h,cc}`

- 从纯流式改为 **function calling 协议循环**：工具轮用非流式（可靠解析 `tool_calls`），逐个执行工具并以 `role:"tool"` 回传，循环直到无工具调用得到最终回复
- 新增属性（均 QSettings 持久化，组名 `AIChat`）：
  - `vehicleControlEnabled`（默认 false）：是否允许 AI 控制飞机
  - `settingsControlEnabled`（默认 false）：是否允许 AI 修改设置（与飞机控制独立）
  - `confirmActions`（默认 true）：控制类/写入类操作执行前是否弹窗确认
- 请求体合并飞机工具 + 设置工具；只读工具始终带，写入类工具仅在对应开关开启时带
- 工具路由 `_executeTool()` 按工具名分发到 VehicleTools / SettingsTools
- 确认判定 `_toolNeedsConfirmation()`：控制类 + 设置写入类受 confirmActions 约束；需确认时 emit `confirmationRequested`，等 QML 回调 `confirmToolCall(callId, approved)`
- 对话区内联显示 `[Tool] ...` / `[Result] ...`，全过程可见可追溯
- 系统提示词补充飞机控制与设置操作的指引

## 四、QML 面板

`qgroundcontrol/src/FlyView/FlyViewCustomLayer.qml`

- 配置区新增两个开关："允许 AI 控制飞机"、"允许 AI 修改 QGC 设置"
- "执行前需确认"开关在任一控制开启时显示
- 监听 `confirmationRequested`，用 `QGroundControl.showMessageDialog`（Ok/Cancel）弹出确认框，回调 `confirmToolCall`

## 五、CMake

`qgroundcontrol/src/Utilities/Diagnostics/CMakeLists.txt`

- 注册 `AIVehicleTools.h/.cc`、`AISettingsTools.h/.cc`

## 安全设计

- 飞机控制、设置写入各自独立开关，默认均关闭
- 默认逐条确认（confirmActions）
- 飞机：未暴露 emergencyStop/reboot 等高风险指令；控制前检查飞机连接
- 设置：白名单 C++ 硬编码，排除所有通信/链路/固件/安全设置；写入前 validate 校验
- 所有工具调用与结果在对话区可见

## 使用方式

1. 打开 AI 面板，点齿轮配置一个**支持 function calling** 的模型（GPT-4o / DeepSeek / Qwen 等；部分本地小模型不支持工具调用）
2. 按需勾选"允许 AI 控制飞机" / "允许 AI 修改 QGC 设置"
3. 对话即可，例如："现在飞机什么状态""起飞到 10 米""把音量调到 50""距离单位改成英尺"

## 涉及文件

| 文件 | 改动 |
| --- | --- |
| `src/Utilities/Diagnostics/AIVehicleTools.h` | 新增 |
| `src/Utilities/Diagnostics/AIVehicleTools.cc` | 新增 |
| `src/Utilities/Diagnostics/AISettingsTools.h` | 新增 |
| `src/Utilities/Diagnostics/AISettingsTools.cc` | 新增 |
| `src/Utilities/Diagnostics/AIChatService.h` | function calling + 新开关属性 |
| `src/Utilities/Diagnostics/AIChatService.cc` | 协议循环、工具路由、确认逻辑 |
| `src/Utilities/Diagnostics/CMakeLists.txt` | 注册新源文件 |
| `src/FlyView/FlyViewCustomLayer.qml` | 控制/设置开关 + 确认弹窗 |

## 已知边界 / 后续可扩展

- 需模型支持 tool use；不支持时控制/设置功能不可用，普通对话仍可用
- 工具轮非流式，仅最终回复可流式（当前实现统一非流式）
- 设置白名单初版较保守，后续按需扩充（加一行即可）
- 可扩展：参数读写、任务规划、日志诊断等
