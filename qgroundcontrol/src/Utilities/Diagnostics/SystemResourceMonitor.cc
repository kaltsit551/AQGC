#include "SystemResourceMonitor.h"

#include <QtCore/QApplicationStatic>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

Q_APPLICATION_STATIC(SystemResourceMonitor, _sysMonitorInstance);

SystemResourceMonitor::SystemResourceMonitor(QObject *parent)
    : QObject(parent)
{
    connect(&_updateTimer, &QTimer::timeout, this, &SystemResourceMonitor::_update);
    _updateTimer.start(1000);
    _update();
}

SystemResourceMonitor::~SystemResourceMonitor() = default;

SystemResourceMonitor *SystemResourceMonitor::instance()
{
    return _sysMonitorInstance;
}

void SystemResourceMonitor::_update()
{
    _updateCpuUsage();
    _updateMemoryUsage();
    emit updated();
}

void SystemResourceMonitor::_updateCpuUsage()
{
#ifdef Q_OS_WIN
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return;
    }

    auto toU64 = [](const FILETIME &ft) -> quint64 {
        return (static_cast<quint64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    quint64 idle = toU64(idleTime);
    quint64 kernel = toU64(kernelTime);
    quint64 user = toU64(userTime);

    if (_firstCpuRead) {
        _firstCpuRead = false;
    } else {
        quint64 idleDelta = idle - _prevIdleTime;
        quint64 kernelDelta = kernel - _prevKernelTime;
        quint64 userDelta = user - _prevUserTime;
        quint64 totalDelta = kernelDelta + userDelta;

        if (totalDelta > 0) {
            _cpuUsage = (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta)) * 100.0;
        }
    }

    _prevIdleTime = idle;
    _prevKernelTime = kernel;
    _prevUserTime = user;
#endif
}

void SystemResourceMonitor::_updateMemoryUsage()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&memInfo)) {
        return;
    }

    _totalMemoryMB = static_cast<double>(memInfo.ullTotalPhys) / (1024.0 * 1024.0);
    double usedBytes = static_cast<double>(memInfo.ullTotalPhys - memInfo.ullAvailPhys);
    _memoryUsageMB = usedBytes / (1024.0 * 1024.0);
    _memoryUsagePercent = (_memoryUsageMB / _totalMemoryMB) * 100.0;
#endif
}
