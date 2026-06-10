#include "AISettingsTools.h"

#include "SettingsManager.h"
#include "Fact.h"

#include <QtCore/QJsonValue>
#include <QtCore/QVariant>

namespace {
QJsonObject makeFunction(const QString &name, const QString &description, const QJsonObject &parameters)
{
    QJsonObject function;
    function[QStringLiteral("name")] = name;
    function[QStringLiteral("description")] = description;
    function[QStringLiteral("parameters")] = parameters;

    QJsonObject tool;
    tool[QStringLiteral("type")] = QStringLiteral("function");
    tool[QStringLiteral("function")] = function;
    return tool;
}

QJsonObject okResult(const QString &message)
{
    QJsonObject result;
    result[QStringLiteral("success")] = true;
    result[QStringLiteral("message")] = message;
    return result;
}

QJsonObject errorResult(const QString &message)
{
    QJsonObject result;
    result[QStringLiteral("success")] = false;
    result[QStringLiteral("error")] = message;
    return result;
}
}

AISettingsTools::AISettingsTools(QObject *parent)
    : QObject(parent)
{
}

const QMap<QString, QSet<QString>> &AISettingsTools::_whitelist()
{
    // Hard-coded safe settings only. Deliberately excludes comms, link,
    // firmware, RTK/NTRIP, RemoteID, joystick and video stream URL/source.
    static const QMap<QString, QSet<QString>> map = {
        {QStringLiteral("appSettings"), {
            QStringLiteral("audioMuted"),
            QStringLiteral("audioVolume"),
            QStringLiteral("uiScalePercent"),
            QStringLiteral("indoorPalette"),
            QStringLiteral("batteryPercentRemainingAnnounce"),
            QStringLiteral("defaultMissionItemAltitude"),
            QStringLiteral("useChecklist"),
            QStringLiteral("enforceChecklist"),
        }},
        {QStringLiteral("unitsSettings"), {
            QStringLiteral("horizontalDistanceUnits"),
            QStringLiteral("verticalDistanceUnits"),
            QStringLiteral("areaUnits"),
            QStringLiteral("speedUnits"),
            QStringLiteral("temperatureUnits"),
            QStringLiteral("weightUnits"),
        }},
        {QStringLiteral("flyViewSettings"), {
            QStringLiteral("keepMapCenteredOnVehicle"),
            QStringLiteral("showSimpleCameraControl"),
            QStringLiteral("showObstacleDistanceOverlay"),
            QStringLiteral("guidedMinimumAltitude"),
            QStringLiteral("guidedMaximumAltitude"),
            QStringLiteral("maxGoToLocationDistance"),
        }},
        {QStringLiteral("flightMapSettings"), {
            QStringLiteral("mapProvider"),
            QStringLiteral("mapType"),
        }},
        {QStringLiteral("videoSettings"), {
            QStringLiteral("gridLines"),
            QStringLiteral("videoFit"),
            QStringLiteral("aspectRatio"),
        }},
    };
    return map;
}

bool AISettingsTools::_isWhitelisted(const QString &group, const QString &name)
{
    const auto &map = _whitelist();
    auto it = map.constFind(group);
    return it != map.constEnd() && it->contains(name);
}

bool AISettingsTools::isSettingsTool(const QString &name)
{
    return name == QLatin1String("list_settings")
        || name == QLatin1String("get_setting")
        || name == QLatin1String("set_setting");
}

bool AISettingsTools::isWriteTool(const QString &name)
{
    return name == QLatin1String("set_setting");
}

Fact *AISettingsTools::_factFor(const QString &group, const QString &name) const
{
    if (!_isWhitelisted(group, name)) {
        return nullptr;
    }
    QObject *groupObj = SettingsManager::instance()->property(group.toUtf8().constData()).value<QObject *>();
    if (!groupObj) {
        return nullptr;
    }
    return groupObj->property(name.toUtf8().constData()).value<Fact *>();
}

// __DESCRIBE_PLACEHOLDER__
QJsonObject AISettingsTools::_describeFact(const QString &group, const QString &name, Fact *fact) const
{
    QJsonObject obj;
    obj[QStringLiteral("group")] = group;
    obj[QStringLiteral("name")] = name;
    if (!fact) {
        obj[QStringLiteral("error")] = QStringLiteral("not available");
        return obj;
    }
    obj[QStringLiteral("value")] = QJsonValue::fromVariant(fact->rawValue());
    obj[QStringLiteral("value_string")] = fact->cookedValueString();
    const QString desc = fact->shortDescription();
    if (!desc.isEmpty()) {
        obj[QStringLiteral("description")] = desc;
    }
    const QString units = fact->cookedUnits();
    if (!units.isEmpty()) {
        obj[QStringLiteral("units")] = units;
    }
    const QStringList enumStrings = fact->enumStrings();
    if (!enumStrings.isEmpty()) {
        obj[QStringLiteral("allowed_values")] = QJsonArray::fromStringList(enumStrings);
    }
    return obj;
}

QJsonArray AISettingsTools::toolDefinitions(bool includeWrite)
{
    QJsonArray tools;

    {
        QJsonObject props;
        QJsonObject group; group[QStringLiteral("type")] = QStringLiteral("string");
        group[QStringLiteral("description")] = QStringLiteral("Optional settings group to filter by (e.g. appSettings, unitsSettings, flyViewSettings, flightMapSettings, videoSettings). Omit to list all.");
        props[QStringLiteral("group")] = group;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        tools.append(makeFunction(QStringLiteral("list_settings"),
            QStringLiteral("List the QGroundControl application settings the assistant is allowed to read or change, "
                           "with each setting's current value, description, units and allowed values. "
                           "Call this to discover what can be configured."), params));
    }

    {
        QJsonObject props;
        QJsonObject group; group[QStringLiteral("type")] = QStringLiteral("string");
        group[QStringLiteral("description")] = QStringLiteral("Settings group name.");
        QJsonObject name; name[QStringLiteral("type")] = QStringLiteral("string");
        name[QStringLiteral("description")] = QStringLiteral("Setting name within the group.");
        props[QStringLiteral("group")] = group;
        props[QStringLiteral("name")] = name;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("group"), QStringLiteral("name")};
        tools.append(makeFunction(QStringLiteral("get_setting"),
            QStringLiteral("Read a single QGroundControl setting's current value and metadata."), params));
    }

    if (includeWrite) {
        QJsonObject props;
        QJsonObject group; group[QStringLiteral("type")] = QStringLiteral("string");
        group[QStringLiteral("description")] = QStringLiteral("Settings group name.");
        QJsonObject name; name[QStringLiteral("type")] = QStringLiteral("string");
        name[QStringLiteral("description")] = QStringLiteral("Setting name within the group.");
        QJsonObject value; value[QStringLiteral("type")] = QStringLiteral("string");
        value[QStringLiteral("description")] = QStringLiteral("New value. For enum settings use one of the allowed_values strings; for numbers/booleans pass the value as text (e.g. \"50\", \"true\").");
        props[QStringLiteral("group")] = group;
        props[QStringLiteral("name")] = name;
        props[QStringLiteral("value")] = value;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("group"), QStringLiteral("name"), QStringLiteral("value")};
        tools.append(makeFunction(QStringLiteral("set_setting"),
            QStringLiteral("Change a QGroundControl application setting. Only whitelisted settings can be changed. "
                           "Prefer reading the setting first to learn its allowed values."), params));
    }

    return tools;
}

// __EXECUTE_PLACEHOLDER__
QJsonObject AISettingsTools::execute(const QString &name, const QJsonObject &args)
{
    if (name == QLatin1String("list_settings")) {
        return _listSettings(args);
    } else if (name == QLatin1String("get_setting")) {
        return _getSetting(args);
    } else if (name == QLatin1String("set_setting")) {
        return _setSetting(args);
    }
    return errorResult(QStringLiteral("Unknown settings tool: ") + name);
}

QJsonObject AISettingsTools::_listSettings(const QJsonObject &args)
{
    const QString filterGroup = args.value(QStringLiteral("group")).toString();

    QJsonArray settings;
    const auto &map = _whitelist();
    for (auto git = map.constBegin(); git != map.constEnd(); ++git) {
        if (!filterGroup.isEmpty() && git.key() != filterGroup) {
            continue;
        }
        // Sort names for stable output.
        QStringList names = git.value().values();
        names.sort();
        for (const QString &name : names) {
            settings.append(_describeFact(git.key(), name, _factFor(git.key(), name)));
        }
    }

    QJsonObject result;
    result[QStringLiteral("settings")] = settings;
    if (settings.isEmpty() && !filterGroup.isEmpty()) {
        result[QStringLiteral("note")] = QStringLiteral("No whitelisted settings in group '%1'.").arg(filterGroup);
    }
    return result;
}

QJsonObject AISettingsTools::_getSetting(const QJsonObject &args)
{
    const QString group = args.value(QStringLiteral("group")).toString();
    const QString name = args.value(QStringLiteral("name")).toString();
    if (!_isWhitelisted(group, name)) {
        return errorResult(QStringLiteral("Setting '%1/%2' is not readable by the assistant.").arg(group, name));
    }
    Fact *fact = _factFor(group, name);
    if (!fact) {
        return errorResult(QStringLiteral("Setting '%1/%2' is not available.").arg(group, name));
    }
    return _describeFact(group, name, fact);
}

QJsonObject AISettingsTools::_setSetting(const QJsonObject &args)
{
    const QString group = args.value(QStringLiteral("group")).toString();
    const QString name = args.value(QStringLiteral("name")).toString();
    const QString valueStr = args.value(QStringLiteral("value")).toVariant().toString();

    if (!_isWhitelisted(group, name)) {
        return errorResult(QStringLiteral("Setting '%1/%2' cannot be changed by the assistant.").arg(group, name));
    }
    Fact *fact = _factFor(group, name);
    if (!fact) {
        return errorResult(QStringLiteral("Setting '%1/%2' is not available.").arg(group, name));
    }

    // For enum settings, allow setting by the human-readable enum string.
    const QStringList enumStrings = fact->enumStrings();
    if (!enumStrings.isEmpty()) {
        const int idx = enumStrings.indexOf(valueStr);
        if (idx >= 0) {
            const QVariantList enumValues = fact->enumValues();
            if (idx < enumValues.size()) {
                fact->setRawValue(enumValues.at(idx));
                return okResult(QStringLiteral("Set %1/%2 to %3.").arg(group, name, valueStr));
            }
        }
        // Fall through: maybe the model passed the raw value directly.
    }

    // Validate/convert against the Fact's type before applying.
    QString errorString = fact->validate(valueStr, false);
    if (!errorString.isEmpty()) {
        QString hint = errorString;
        if (!enumStrings.isEmpty()) {
            hint += QStringLiteral(" Allowed values: ") + enumStrings.join(QStringLiteral(", "));
        }
        return errorResult(QStringLiteral("Invalid value for %1/%2: %3").arg(group, name, hint));
    }

    fact->setCookedValue(fact->clamp(valueStr));
    return okResult(QStringLiteral("Set %1/%2 to %3.").arg(group, name, fact->cookedValueString()));
}


