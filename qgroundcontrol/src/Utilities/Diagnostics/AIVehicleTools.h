#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

class Vehicle;
class FactGroup;

/// Exposes vehicle telemetry (read) and guided control (write) to the AI
/// assistant as OpenAI-style function-calling tools. Always operates on the
/// currently active vehicle obtained from MultiVehicleManager.
class AIVehicleTools : public QObject
{
    Q_OBJECT

public:
    explicit AIVehicleTools(QObject *parent = nullptr);

    /// OpenAI "tools" array. If includeControl is false, only read-only tools
    /// are returned (control tools are omitted entirely).
    static QJsonArray toolDefinitions(bool includeControl);

    /// True if the named tool commands the vehicle (vs. read-only).
    static bool isControlTool(const QString &name);

    /// Execute a tool by name with JSON arguments. Returns a JSON object with
    /// the result (or an "error" field). Read tools and control tools both go
    /// through here; the caller is responsible for confirmation gating of
    /// control tools before calling this.
    QJsonObject execute(const QString &name, const QJsonObject &args);

private:
    Vehicle *_activeVehicle() const;
    static double _factValue(FactGroup *group, const QString &fact);

    QJsonObject _getVehicleStatus();
    QJsonObject _takeoff(const QJsonObject &args);
    QJsonObject _land();
    QJsonObject _returnToLaunch();
    QJsonObject _gotoLocation(const QJsonObject &args);
    QJsonObject _changeAltitude(const QJsonObject &args);
    QJsonObject _setFlightMode(const QJsonObject &args);
    QJsonObject _arm(bool arm);
    QJsonObject _pause();
};
