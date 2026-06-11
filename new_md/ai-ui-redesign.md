# AI 对话界面美化与独立

**日期**：2026-06-10
**分支**：master

## 概述

对 AI 对话界面进行了多轮 UI 优化，并将其从 `FlyViewCustomLayer.qml` 抽取为独立组件。涵盖：结构化气泡、Markdown 渲染、现代聊天风布局、全局微软雅黑字体、经典蓝配色、开源图标、组件独立化。

## 改动内容

### 1. 结构化气泡对话（C++ + QML）
- `AIChatService` 新增 `conversationModel`（QVariantList），每条消息为结构化 map（role + 内容），替换原来拼接整段字符串的方式
- 语义化 push 方法：`_pushUser/_pushAssistant/_pushToolCall/_pushToolResult/_pushError/_pushStatus`
- QML 用 ListView + 按 role 切换的气泡 delegate：用户靠右、AI 靠左、工具调用/结果折叠卡片、错误红色、状态居中

### 2. P0 Markdown 渲染
- AI 气泡用 `TextEdit.MarkdownText`，支持加粗/列表/标题/行内代码/代码块/链接
- 链接可点击（`Qt.openUrlExternally`）
- 用户/错误气泡保持纯文本

### 3. P1 布局重构
- 气泡尺寸改用 `contentWidth`/`contentHeight`，修正原单行 `implicitWidth` 估算导致的多行不贴合
- 统一间距常量（`_bubblePad`/`_bubbleRadius`）
- 工具卡片等宽字体由硬编码 "Courier New" 改为 `ScreenTools.fixedFontFamily`

### 4. 元宝风格界面
- 空对话欢迎页：居中大标题问候 + 建议气泡 chip（点击直接发送）
- 大圆角输入卡片：多行输入（Enter 发送、Shift+Enter 换行）+ 右下圆形发送按钮（回复中变红色停止键）
- 底部 "AI-generated content, for reference only" 提示

### 5. 全局字体改微软雅黑
- `ScreenToolsController::normalFontFamily()`：Windows 平台返回 "Microsoft YaHei"，其他平台保持 "Open Sans"，韩语特例保留

### 6. 经典蓝配色
- AI 面板内定义独立配色（不动全局 qgcPal），随明/暗主题切换：
  - 主色 `#2563EB`，用户气泡蓝底白字
  - AI 气泡亮 `#EEF1F5` / 暗 `#2B2F36`
  - 工具卡片、边框、chip 配套中性色
- 解决了原来用户/AI 气泡套用 qgcPal 偏灰语义色导致的"灰扑扑"问题

### 7. 开源图标（Lucide, ISC 许可）
- 新增 `qgroundcontrol/resources/AIChat/` 目录，9 个线性 SVG（send/stop/settings/trash/close/wrench/terminal/copy/sparkles）
- `src/QmlControls/CMakeLists.txt` 用独立资源前缀 `/AIChat` 注册（不污染 InstrumentValueIcons 图标选择器）
- 替换发送/停止/配置/清空/关闭/工具图标；标题栏改用 sparkles 徽标

### 8. AI 界面独立成组件
- 新建 `qgroundcontrol/src/FlyView/FlyViewAIChatPanel.qml`（自包含：按钮 + 气泡 delegate + 配色 + 面板 + 确认弹窗 + 自带 QGCPalette）
- `FlyViewCustomLayer.qml` 从 1046 行精简到 395 行，删除全部 AI 代码，改为一行 `FlyViewAIChatPanel { anchors.fill: parent }`
- `src/FlyView/CMakeLists.txt` 注册新 QML 文件
- 纯重构，行为零变化

## 涉及文件

| 文件 | 改动 |
| --- | --- |
| `src/Utilities/Diagnostics/AIChatService.h/.cc` | conversationModel + 语义化 push |
| `src/QmlControls/ScreenToolsController.cc` | 全局字体微软雅黑 |
| `src/QmlControls/CMakeLists.txt` | 注册 /AIChat 图标资源 |
| `src/FlyView/FlyViewAIChatPanel.qml` | 新增：独立 AI 界面组件 |
| `src/FlyView/FlyViewCustomLayer.qml` | 移除 AI 代码，改为实例化 |
| `src/FlyView/CMakeLists.txt` | 注册新 QML 文件 |
| `resources/AIChat/*.svg` | 新增：9 个 Lucide 图标 |

## 构建备注
- 新增 QML 文件 / 图标资源需 CMake 重新配置
- vcvarsall 通过 `cmd.exe /c` 绝对路径调用、不重定向输出、`if not defined INCLUDE` 守卫

## 已知边界
- `TextEdit.MarkdownText` 对复杂表格支持有限，常规格式足够
- 图标依赖 Lucide SVG（已下载入库，离线可用）
