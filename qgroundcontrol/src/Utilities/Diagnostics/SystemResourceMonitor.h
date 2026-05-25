#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

class SystemResourceMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY updated)
    Q_PROPERTY(double memoryUsageMB READ memoryUsageMB NOTIFY updated)
    Q_PROPERTY(double memoryUsagePercent READ memoryUsagePercent NOTIFY updated)
    Q_PROPERTY(double totalMemoryMB READ totalMemoryMB NOTIFY updated)

public:
    explicit SystemResourceMonitor(QObject *parent = nullptr);
    ~SystemResourceMonitor() override;

    static SystemResourceMonitor *instance();

    double cpuUsage() const { return _cpuUsage; }
    double memoryUsageMB() const { return _memoryUsageMB; }
    double memoryUsagePercent() const { return _memoryUsagePercent; }
    double totalMemoryMB() const { return _totalMemoryMB; }

signals:
    void updated();

private slots:
    void _update();

private:
    void _updateCpuUsage();
    void _updateMemoryUsage();

    QTimer _updateTimer;
    double _cpuUsage = 0.0;
    double _memoryUsageMB = 0.0;
    double _memoryUsagePercent = 0.0;
    double _totalMemoryMB = 0.0;

#ifdef Q_OS_WIN
    quint64 _prevIdleTime = 0;
    quint64 _prevKernelTime = 0;
    quint64 _prevUserTime = 0;
    bool _firstCpuRead = true;
#endif
};
