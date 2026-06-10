#include "AIParameterTools.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "ParameterManager.h"
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

// Exact parameter names that may be written. Covers common PX4 + ArduPilot
// names; unknown firmware-specific names simply won't match (safe default).
const QSet<QString> &writeExactNames()
{
    static const QSet<QString> names = {
        // Return / land
        QStringLiteral("RTL_ALT"),            // ArduPilot return altitude (cm)
        QStringLiteral("RTL_RETURN_ALT"),     // PX4
        QStringLiteral("RTL_DESCEND_ALT"),    // PX4
        QStringLiteral("LAND_SPEED"),         // ArduPilot / PX4
        // Speed limits
        QStringLiteral("PILOT_SPEED_UP"),     // ArduPilot
        QStringLiteral("PILOT_SPEED_DN"),     // ArduPilot
        QStringLiteral("WPNAV_SPEED"),        // ArduPilot horizontal
        QStringLiteral("WPNAV_SPEED_UP"),
        QStringLiteral("WPNAV_SPEED_DN"),
        // Geofence
        QStringLiteral("FENCE_ALT_MAX"),
        QStringLiteral("FENCE_RADIUS"),
        // Battery protection thresholds
        QStringLiteral("BATT_LOW_VOLT"),
        QStringLiteral("BATT_CRT_VOLT"),
        QStringLiteral("BATT_LOW_MAH"),
        QStringLiteral("BATT_CRT_MAH"),
    };
    return names;
}

// Allowed write prefixes (firmware-portable families considered safe to tune).
const QStringList &writePrefixes()
{
    static const QStringList prefixes = {
        QStringLiteral("MPC_XY_VEL"),     // PX4 horizontal velocity limits
        QStringLiteral("MPC_Z_VEL"),      // PX4 vertical velocity limits
        QStringLiteral("MPC_LAND_SPEED"), // PX4 land speed
    };
    return prefixes;
}
}

AIParameterTools::AIParameterTools(QObject *parent)
    : QObject(parent)
{
}

bool AIParameterTools::isParameterTool(const QString &name)
{
    return name == QLatin1String("search_parameters")
        || name == QLatin1String("get_parameter")
        || name == QLatin1String("set_parameter");
}

bool AIParameterTools::isWriteTool(const QString &name)
{
    return name == QLatin1String("set_parameter");
}

bool AIParameterTools::_isWriteAllowed(const QString &paramName)
{
    if (writeExactNames().contains(paramName)) {
        return true;
    }
    for (const QString &prefix : writePrefixes()) {
        if (paramName.startsWith(prefix)) {
            return true;
        }
    }
    return false;
}

Vehicle *AIParameterTools::_activeVehicle() const
{
    return MultiVehicleManager::instance()->activeVehicle();
}

// __DESCRIBE_PLACEHOLDER__
QJsonObject AIParameterTools::_describeParameter(const QString &name, Fact *fact) const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    if (!fact) {
        obj[QStringLiteral("error")] = QStringLiteral("not available");
        return obj;
    }
    obj[QStringLiteral("value")] = QJsonValue::fromVariant(fact->rawValue());
    obj[QStringLiteral("value_string")] = fact->cookedValueString();
    obj[QStringLiteral("writable")] = _isWriteAllowed(name);

    const QString desc = fact->shortDescription();
    if (!desc.isEmpty()) {
        obj[QStringLiteral("description")] = desc;
    }
    const QString units = fact->cookedUnits();
    if (!units.isEmpty()) {
        obj[QStringLiteral("units")] = units;
    }
    if (!fact->minIsDefaultForType()) {
        obj[QStringLiteral("min")] = fact->cookedMinString();
    }
    if (!fact->maxIsDefaultForType()) {
        obj[QStringLiteral("max")] = fact->cookedMaxString();
    }
    const QStringList enumStrings = fact->enumStrings();
    if (!enumStrings.isEmpty()) {
        obj[QStringLiteral("allowed_values")] = QJsonArray::fromStringList(enumStrings);
    }
    return obj;
}

QJsonArray AIParameterTools::toolDefinitions(bool includeWrite)
{
    QJsonArray tools;

    {
        QJsonObject props;
        QJsonObject keyword; keyword[QStringLiteral("type")] = QStringLiteral("string");
        keyword[QStringLiteral("description")] = QStringLiteral("Case-insensitive substring or prefix to match against parameter names (e.g. \"RTL\", \"BATT\", \"SPEED\").");
        props[QStringLiteral("keyword")] = keyword;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("keyword")};
        tools.append(makeFunction(QStringLiteral("search_parameters"),
            QStringLiteral("Search the connected flight controller's parameters by keyword. Returns matching "
                           "parameter names. Use this to find a parameter before reading or changing it."), params));
    }

    {
        QJsonObject props;
        QJsonObject name; name[QStringLiteral("type")] = QStringLiteral("string");
        name[QStringLiteral("description")] = QStringLiteral("Exact parameter name.");
        props[QStringLiteral("name")] = name;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("name")};
        tools.append(makeFunction(QStringLiteral("get_parameter"),
            QStringLiteral("Read a single flight controller parameter: current value, description, units, "
                           "min/max and whether it is writable by the assistant."), params));
    }

    if (includeWrite) {
        QJsonObject props;
        QJsonObject name; name[QStringLiteral("type")] = QStringLiteral("string");
        name[QStringLiteral("description")] = QStringLiteral("Exact parameter name.");
        QJsonObject value; value[QStringLiteral("type")] = QStringLiteral("string");
        value[QStringLiteral("description")] = QStringLiteral("New value as text (e.g. \"30\", \"5.5\"). Will be validated against the parameter's type and range.");
        props[QStringLiteral("name")] = name;
        props[QStringLiteral("value")] = value;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("name"), QStringLiteral("value")};
        tools.append(makeFunction(QStringLiteral("set_parameter"),
            QStringLiteral("Change a flight controller parameter. Only a whitelisted set of safe parameters can be "
                           "changed; all others are read-only. The change is sent to the vehicle. Always read the "
                           "parameter first to understand its meaning and range. This is a high-risk operation."), params));
    }

    return tools;
}

// __EXECUTE_PLACEHOLDER__
QJsonObject AIParameterTools::execute(const QString &name, const QJsonObject &args)
{
    if (name == QLatin1String("search_parameters")) {
        return _searchParameters(args);
    } else if (name == QLatin1String("get_parameter")) {
        return _getParameter(args);
    } else if (name == QLatin1String("set_parameter")) {
        return _setParameter(args);
    }
    return errorResult(QStringLiteral("Unknown parameter tool: ") + name);
}

QJsonObject AIParameterTools::_searchParameters(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    ParameterManager *pm = vehicle->parameterManager();
    if (!pm || !pm->parametersReady()) {
        return errorResult(QStringLiteral("Parameters are not loaded yet."));
    }

    const QString keyword = args.value(QStringLiteral("keyword")).toString().trimmed();
    if (keyword.isEmpty()) {
        return errorResult(QStringLiteral("keyword is required."));
    }

    const int compId = ParameterManager::defaultComponentId;
    const QStringList allNames = pm->parameterNames(compId);

    QJsonArray matches;
    int total = 0;
    for (const QString &n : allNames) {
        if (n.contains(keyword, Qt::CaseInsensitive)) {
            total++;
            if (matches.size() < kMaxSearchResults) {
                matches.append(n);
            }
        }
    }

    QJsonObject result;
    result[QStringLiteral("matches")] = matches;
    result[QStringLiteral("match_count")] = total;
    if (total > matches.size()) {
        result[QStringLiteral("note")] = QStringLiteral("Showing first %1 of %2 matches; refine the keyword.")
                                             .arg(matches.size()).arg(total);
    }
    if (total == 0) {
        result[QStringLiteral("note")] = QStringLiteral("No parameters matched '%1'.").arg(keyword);
    }
    return result;
}

QJsonObject AIParameterTools::_getParameter(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    ParameterManager *pm = vehicle->parameterManager();
    if (!pm || !pm->parametersReady()) {
        return errorResult(QStringLiteral("Parameters are not loaded yet."));
    }

    const QString name = args.value(QStringLiteral("name")).toString();
    const int compId = ParameterManager::defaultComponentId;
    if (!pm->parameterExists(compId, name)) {
        return errorResult(QStringLiteral("Parameter '%1' does not exist on this vehicle.").arg(name));
    }
    return _describeParameter(name, pm->getParameter(compId, name));
}

QJsonObject AIParameterTools::_setParameter(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    ParameterManager *pm = vehicle->parameterManager();
    if (!pm || !pm->parametersReady()) {
        return errorResult(QStringLiteral("Parameters are not loaded yet."));
    }

    const QString name = args.value(QStringLiteral("name")).toString();
    const QString valueStr = args.value(QStringLiteral("value")).toVariant().toString();
    const int compId = ParameterManager::defaultComponentId;

    if (!_isWriteAllowed(name)) {
        return errorResult(QStringLiteral("Parameter '%1' is read-only for the assistant (not in the safe whitelist).").arg(name));
    }
    if (!pm->parameterExists(compId, name)) {
        return errorResult(QStringLiteral("Parameter '%1' does not exist on this vehicle.").arg(name));
    }

    Fact *fact = pm->getParameter(compId, name);
    if (!fact) {
        return errorResult(QStringLiteral("Parameter '%1' is not available.").arg(name));
    }

    const QString errorString = fact->validate(valueStr, false);
    if (!errorString.isEmpty()) {
        return errorResult(QStringLiteral("Invalid value for %1: %2").arg(name, errorString));
    }

    const QVariant oldValue = fact->rawValue();
    fact->setCookedValue(fact->clamp(valueStr));
    // Writing the Fact triggers ParameterManager to send PARAM_SET to the vehicle.
    return okResult(QStringLiteral("Set %1 from %2 to %3 (sent to vehicle).")
                        .arg(name, oldValue.toString(), fact->cookedValueString()));
}


