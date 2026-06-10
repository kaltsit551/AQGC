# AI 扩展：读写飞控参数（FC Parameters）

**日期**：2026-06-10
**分支**：master

## 功能概述

给 AI 助手新增"读取/修改**飞控参数**"的能力——操作的是 PX4/ArduPilot 固件里的参数（如 RTL_ALT、PILOT_SPEED_UP、BATT_LOW_VOLT、FENCE_*）。

注意区分两套系统：
- **QGC 应用设置**（之前已做，AISettingsTools）：地面站软件自己的设置，存本地
- **飞控参数**（本次，AIParameterTools）：存在飞控固件里，通过 MAVLink PARAM_SET 下发

本功能采用最严格的安全组合：**读不限 + 写白名单 + 强制逐条确认**。

## 飞控参数系统

- 入口：`vehicle->parameterManager()`
- 关键 API：`parametersReady()`、`parameterNames(compId)`、`parameterExists()`、`getParameter()` → `Fact*`，`defaultComponentId = -1`
- **写入即下发**：`fact->setRawValue(value)` 触发 ParameterManager 自动发送 PARAM_SET 给飞控（带重试），与 ParameterEditor 同一机制

## 改动清单

### 1. 新建 AIParameterTools

`qgroundcontrol/src/Utilities/Diagnostics/AIParameterTools.{h,cc}`

- 普通 QObject，由 AIChatService 持有；每次从 `MultiVehicleManager::instance()->activeVehicle()->parameterManager()` 取
- 三个工具：
  - `search_parameters(keyword)`：按关键词/子串搜参数名，最多返回 60 条（超出提示精化），避免一次性返回上千参数
  - `get_parameter(name)`：读单个参数当前值、描述、单位、min/max、可选枚举、是否可写——**读取不限制**
  - `set_parameter(name, value)`：改参数——白名单校验 + `Fact::validate()` 类型/范围校验 + `setCookedValue(clamp(...))` 下发
- 写白名单（因固件差异采用"精确名 + 前缀"）：
  - 精确名：`RTL_ALT`、`RTL_RETURN_ALT`、`RTL_DESCEND_ALT`、`LAND_SPEED`、`PILOT_SPEED_UP`、`PILOT_SPEED_DN`、`WPNAV_SPEED`、`WPNAV_SPEED_UP`、`WPNAV_SPEED_DN`、`FENCE_ALT_MAX`、`FENCE_RADIUS`、`BATT_LOW_VOLT`、`BATT_CRT_VOLT`、`BATT_LOW_MAH`、`BATT_CRT_MAH`
  - 前缀：`MPC_XY_VEL`、`MPC_Z_VEL`、`MPC_LAND_SPEED`（PX4 速度限制族）
  - **明确排除**：机架类型、传感器校准、电机方向、固件/通信类参数；不在白名单一律只读
- 前置检查：activeVehicle 存在 + `parametersReady()`（参数加载完成），否则返回友好错误

### 2. 接入 AIChatService

`qgroundcontrol/src/Utilities/Diagnostics/AIChatService.{h,cc}`

- 持有 `AIParameterTools *_paramTools`
- 新增属性 `parameterControlEnabled`（QSettings 持久化，默认 false），与 vehicleControlEnabled / settingsControlEnabled 独立
- `_startRequest()` 合并参数工具：search/get 始终带；`set_parameter` 仅在开关开启时带
- `_executeTool()` 路由扩展：`AIParameterTools::isParameterTool(name)` → 调 `_paramTools->execute()`
- **`_toolNeedsConfirmation()` 关键改动**：`set_parameter` **始终需要确认**，无视全局 confirmActions 开关（参数写入风险最高）
- 系统提示词补充飞控参数读写指引，强调先读后写、高风险

### 3. CMake

`qgroundcontrol/src/Utilities/Diagnostics/CMakeLists.txt` 注册 `AIParameterTools.h/.cc`

### 4. QML

`qgroundcontrol/src/FlyView/FlyViewCustomLayer.qml`

- 配置区新增开关"允许 AI 修改飞控参数（高风险）"，绑定 `parameterControlEnabled`
- 确认弹窗通用，`set_parameter` 自动复用

## 安全设计（最严格组合）

- 独立开关 `parameterControlEnabled`，默认关闭
- 写参数**强制逐条确认**，即使关掉全局确认开关也必弹窗
- 写白名单 C++ 硬编码，非白名单参数只读
- 写入前 `parametersReady()` + `validate()` 校验（类型/范围）
- 排除机架/校准/电机/固件/通信等危险参数
- 所有读写在对话区可见（[Tool]/[Result]）

## 涉及文件

| 文件 | 改动 |
| --- | --- |
| `src/Utilities/Diagnostics/AIParameterTools.h` | 新增 |
| `src/Utilities/Diagnostics/AIParameterTools.cc` | 新增 |
| `src/Utilities/Diagnostics/AIChatService.h` | parameterControlEnabled 属性 + 成员 |
| `src/Utilities/Diagnostics/AIChatService.cc` | 工具合并/路由、强制确认逻辑、系统提示 |
| `src/Utilities/Diagnostics/CMakeLists.txt` | 注册新源文件 |
| `src/FlyView/FlyViewCustomLayer.qml` | 飞控参数开关 |

## 使用方式

1. 打开 AI 面板，齿轮里勾选"允许 AI 修改飞控参数（高风险）"
2. 连接飞控（或 SITL 仿真），等参数下载完成
3. 对话，例如："查一下 RTL 相关参数""把返航高度改成 30 米"

## 已知边界

- 飞行中部分固件会跳过参数下载（parameterDownloadSkipped），此时读取可能不全，会如实提示
- 写白名单初版保守，后续按需扩充（加一行）
- 需模型支持 function calling

## 构建备注

本机 VS 2026 (insiders) 的 vcvarsall.bat 存在环境怪癖：在重定向其输出或修改 PATH 后调用时，可能不设置 INCLUDE/LIB，导致编译找不到系统头文件。可靠做法：通过 `cmd.exe /c` 用绝对路径调用批处理、不重定向 vcvarsall 输出、不预改 PATH，并在编译前用 `if not defined INCLUDE` 守卫确认环境就绪。
