# 性能信息指示器 (Performance Indicator)

## 日期
2026-05-25

## 功能描述
在 QGC 工具栏右侧添加性能信息指示器，实时显示：
- FPS（帧率）：通过 QML FrameAnimation 每秒计算
- CPU 使用率：通过 Windows API GetSystemTimes() 获取
- 内存使用率：通过 Windows API GlobalMemoryStatusEx() 获取

点击指示器可展开详情面板，显示更详细的性能数据（FPS、CPU%、已用内存 MB、内存占比、总内存）。

## 新增文件
| 文件 | 说明 |
|------|------|
| `src/Utilities/Diagnostics/SystemResourceMonitor.h` | C++ 后端头文件，定义 CPU/内存监控单例类 |
| `src/Utilities/Diagnostics/SystemResourceMonitor.cc` | C++ 后端实现，Windows API 采集系统资源 |
| `src/Toolbar/PerformanceIndicator.qml` | 工具栏指示器 QML 组件 |
| `src/Toolbar/PerformanceIndicatorPage.qml` | 点击展开的详情页 QML 组件 |

## 修改文件
| 文件 | 修改内容 |
|------|----------|
| `src/Utilities/Diagnostics/CMakeLists.txt` | 注册 SystemResourceMonitor 源文件 |
| `src/QmlControls/QGroundControlQmlGlobal.h` | 添加 systemResourceMonitor Q_PROPERTY |
| `src/QmlControls/QGroundControlQmlGlobal.cc` | 初始化 SystemResourceMonitor 实例 |
| `src/API/QGCCorePlugin.cc` | 在 toolBarIndicators() 中注册 PerformanceIndicator |
| `src/Toolbar/CMakeLists.txt` | 添加两个 QML 文件到 ToolbarModule |

## 技术要点
- SystemResourceMonitor 使用 Q_APPLICATION_STATIC 实现单例，1秒定时器更新数据
- FPS 在 QML 端通过 FrameAnimation + Timer 每秒统计帧数
- 指示器始终显示（不依赖飞行器连接），通过 `showIndicator: true`
- 遵循 QGC 规范：使用 ScreenTools 控制字体大小，QGCPalette 控制颜色
