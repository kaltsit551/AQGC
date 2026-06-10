#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

class Vehicle;
class Fact;

/// Exposes flight-controller parameters (PX4/ArduPilot) to the AI assistant as
/// function-calling tools. Reads are unrestricted; writes are limited to a
/// hard-coded whitelist (exact names + prefixes) and always require user
/// confirmation (enforced by AIChatService).
class AIParameterTools : public QObject
{
    Q_OBJECT

public:
    explicit AIParameterTools(QObject *parent = nullptr);

    /// OpenAI "tools" array. If includeWrite is false, set_parameter is omitted.
    static QJsonArray toolDefinitions(bool includeWrite);

    static bool isParameterTool(const QString &name);
    static bool isWriteTool(const QString &name);

    QJsonObject execute(const QString &name, const QJsonObject &args);

private:
    static bool _isWriteAllowed(const QString &paramName);

    Vehicle *_activeVehicle() const;
    QJsonObject _describeParameter(const QString &name, Fact *fact) const;

    QJsonObject _searchParameters(const QJsonObject &args);
    QJsonObject _getParameter(const QJsonObject &args);
    QJsonObject _setParameter(const QJsonObject &args);

    static constexpr int kMaxSearchResults = 60;
};
