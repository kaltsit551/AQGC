# 添加 AI 对话功能

**日期**：2026-06-10
**分支**：master

## 功能概述

在 QGroundControl 飞行界面（FlyView）中新增了一个内置的 AI 对话面板，支持对接任意 OpenAI 兼容的大模型服务（OpenAI、DeepSeek、通义千问、Kimi、本地 Ollama / vLLM 等），可在监控飞行的同时与大模型对话。

## 主要特性

- **流式输出**：回复以 SSE 流式逐字显示，无需等待整段返回
- **上下文记忆**：维护完整对话历史，支持连续追问
- **通用接口**：使用 OpenAI `/chat/completions` 格式，兼容主流在线及本地服务
- **配置持久化**：Base URL / API Key / Model 通过 QSettings 保存，重启后保留
- **可拖动面板**：基于 `QGCMovableItem`，位置可自由调整
- **中断与清空**：支持 Stop 中止当前回复、Clear 清空整段对话

## 实现方式

沿用项目已有的 ScriptRunner 扩展模式：C++ 单例后端 + QML 全局注册 + 可拖动 QML 面板。

### 1. C++ 后端（新增）

`qgroundcontrol/src/Utilities/Diagnostics/AIChatService.{h,cc}`

- 单例（`QML_SINGLETON` + `Q_APPLICATION_STATIC`），通过 `QGroundControl.aiChatService` 暴露给 QML
- 配置属性 `apiKey` / `baseUrl` / `model`，用 QSettings 持久化（组名 `AIChat`）
  - 默认 Base URL：`https://api.openai.com/v1`
  - 默认 Model：`gpt-4o-mini`
- 状态属性 `conversation`（对话全文）、`busy`（请求中）
- 方法：`sendMessage()` / `clearConversation()` / `stop()`
- 网络：`QNetworkAccessManager` POST 到 `{baseUrl}/chat/completions`，带 `Authorization: Bearer` 头、完整 `messages` 历史和 `stream: true`
- 流式解析：在 `readyRead` 中按 SSE 逐行解析 `data: {...}`，提取 `choices[0].delta.content`；用成员缓冲处理跨数据包的半行

### 2. QML 全局注册

`qgroundcontrol/src/QmlControls/QGroundControlQmlGlobal.{h,cc}`

- 添加前置声明、`Q_MOC_INCLUDE`、`Q_PROPERTY(AIChatService* aiChatService ...)`、getter、成员
- 构造函数初始化 `_aiChatService(AIChatService::instance())`

### 3. CMake 注册

`qgroundcontrol/src/Utilities/Diagnostics/CMakeLists.txt`

- 在 `target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...)` 中加入 `AIChatService.h` / `AIChatService.cc`

### 4. QML 对话面板

`qgroundcontrol/src/FlyView/FlyViewCustomLayer.qml`

- 右上角新增 "AI" 触发按钮（位于 Image / Script 按钮下方）
- 新增 `QGCMovableItem` 对话面板：
  - 标题栏：标题 + 齿轮图标（展开/收起配置）+ 关闭图标
  - 可折叠配置区：Base URL / API Key（密码框）/ Model 三个输入框，绑定到 `aiChatService`
  - 对话显示区：`QGCFlickable` + `QGCLabel` 显示 `conversation`，新内容自动滚动到底部
  - 底部输入栏：输入框（回车发送）+ Send / Stop / Clear 按钮，请求中禁用输入

## 使用方式

1. 飞行界面右上角点击 **AI** 按钮打开面板
2. 点击标题栏齿轮图标，填入 Base URL、API Key、Model
3. 在底部输入框输入消息，回车或点 Send 发送

## 涉及文件

| 文件 | 改动 |
| --- | --- |
| `src/Utilities/Diagnostics/AIChatService.h` | 新增 |
| `src/Utilities/Diagnostics/AIChatService.cc` | 新增 |
| `src/Utilities/Diagnostics/CMakeLists.txt` | 注册源文件 |
| `src/QmlControls/QGroundControlQmlGlobal.h` | 注册 QML 属性 |
| `src/QmlControls/QGroundControlQmlGlobal.cc` | include + 构造初始化 |
| `src/FlyView/FlyViewCustomLayer.qml` | AI 按钮 + 对话面板 |

## 已知边界 / 后续可扩展

- 当前为纯文字对话，AI 暂不能读取无人机飞行数据、参数或日志
- 单条线性对话，无多会话 / 历史管理
- API Key 以明文存于 QSettings（与 QGC 其它凭据存储方式一致）
- 可扩展方向：将飞行状态 / 电池 / GPS / 报错信息喂给 AI，实现飞行诊断辅助
