#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

class Fact;

/// Exposes a whitelisted subset of QGroundControl application settings to the
/// AI assistant as function-calling tools (list / get / set). The whitelist is
/// hard-coded so the AI can never touch comms, link, firmware or safety
/// settings.
class AISettingsTools : public QObject
{
    Q_OBJECT

public:
    explicit AISettingsTools(QObject *parent = nullptr);

    /// OpenAI "tools" array. If includeWrite is false, the set_setting tool is
    /// omitted (read-only list/get are always included).
    static QJsonArray toolDefinitions(bool includeWrite);

    /// True if the named tool belongs to this provider.
    static bool isSettingsTool(const QString &name);
    /// True if the named tool writes a setting (needs confirmation gating).
    static bool isWriteTool(const QString &name);

    QJsonObject execute(const QString &name, const QJsonObject &args);

private:
    static const QMap<QString, QSet<QString>> &_whitelist();
    static bool _isWhitelisted(const QString &group, const QString &name);

    /// Resolve a whitelisted setting to its Fact (nullptr if missing/not allowed).
    Fact *_factFor(const QString &group, const QString &name) const;
    QJsonObject _describeFact(const QString &group, const QString &name, Fact *fact) const;

    QJsonObject _listSettings(const QJsonObject &args);
    QJsonObject _getSetting(const QJsonObject &args);
    QJsonObject _setSetting(const QJsonObject &args);
};
