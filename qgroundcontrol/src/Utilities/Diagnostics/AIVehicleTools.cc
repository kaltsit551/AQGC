#include "AIVehicleTools.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "Fact.h"
#include "FactGroup.h"
#include "QmlObjectListModel.h"

#include <QtPositioning/QGeoCoordinate>

namespace {
// Read-only tools never require confirmation.
const QStringList kControlTools = {
    QStringLiteral("takeoff"),
    QStringLiteral("land"),
    QStringLiteral("return_to_launch"),
    QStringLiteral("goto_location"),
    QStringLiteral("change_altitude"),
    QStringLiteral("set_flight_mode"),
    QStringLiteral("arm"),
    QStringLiteral("disarm"),
    QStringLiteral("pause"),
};

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

QJsonObject noParams()
{
    QJsonObject params;
    params[QStringLiteral("type")] = QStringLiteral("object");
    params[QStringLiteral("properties")] = QJsonObject();
    return params;
}
}

AIVehicleTools::AIVehicleTools(QObject *parent)
    : QObject(parent)
{
}

bool AIVehicleTools::isControlTool(const QString &name)
{
    return kControlTools.contains(name);
}

Vehicle *AIVehicleTools::_activeVehicle() const
{
    return MultiVehicleManager::instance()->activeVehicle();
}

double AIVehicleTools::_factValue(FactGroup *group, const QString &fact)
{
    if (!group) {
        return 0.0;
    }
    Fact *f = group->getFact(fact);
    return f ? f->rawValue().toDouble() : 0.0;
}

// __DEFINITIONS_PLACEHOLDER__
QJsonArray AIVehicleTools::toolDefinitions(bool includeControl)
{
    QJsonArray tools;

    // ---- Read-only ----
    tools.append(makeFunction(
        QStringLiteral("get_vehicle_status"),
        QStringLiteral("Get the current status of the connected drone: connection, armed state, flight mode, "
                       "altitude, speed, battery, GPS, attitude, distance to home, position and vehicle type. "
                       "Call this whenever the user asks about the aircraft's current state."),
        noParams()));

    if (!includeControl) {
        return tools;
    }

    // ---- Control ----
    {
        QJsonObject props;
        QJsonObject alt;
        alt[QStringLiteral("type")] = QStringLiteral("number");
        alt[QStringLiteral("description")] = QStringLiteral("Target altitude above launch in meters.");
        props[QStringLiteral("altitude_m")] = alt;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("altitude_m")};
        tools.append(makeFunction(QStringLiteral("takeoff"),
            QStringLiteral("Command the drone to take off to the given altitude (guided mode)."), params));
    }

    tools.append(makeFunction(QStringLiteral("land"),
        QStringLiteral("Command the drone to land at its current position."), noParams()));

    tools.append(makeFunction(QStringLiteral("return_to_launch"),
        QStringLiteral("Command the drone to return to the launch/home position (RTL)."), noParams()));

    {
        QJsonObject props;
        QJsonObject lat; lat[QStringLiteral("type")] = QStringLiteral("number");
        lat[QStringLiteral("description")] = QStringLiteral("Target latitude in degrees.");
        QJsonObject lon; lon[QStringLiteral("type")] = QStringLiteral("number");
        lon[QStringLiteral("description")] = QStringLiteral("Target longitude in degrees.");
        props[QStringLiteral("latitude")] = lat;
        props[QStringLiteral("longitude")] = lon;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("latitude"), QStringLiteral("longitude")};
        tools.append(makeFunction(QStringLiteral("goto_location"),
            QStringLiteral("Command the drone to fly to the given GPS coordinate (guided mode)."), params));
    }

    {
        QJsonObject props;
        QJsonObject delta; delta[QStringLiteral("type")] = QStringLiteral("number");
        delta[QStringLiteral("description")] = QStringLiteral("Altitude change in meters (positive=up, negative=down).");
        props[QStringLiteral("delta_m")] = delta;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("delta_m")};
        tools.append(makeFunction(QStringLiteral("change_altitude"),
            QStringLiteral("Change the drone's altitude by a relative amount (guided mode)."), params));
    }

    {
        QJsonObject props;
        QJsonObject mode; mode[QStringLiteral("type")] = QStringLiteral("string");
        mode[QStringLiteral("description")] = QStringLiteral("Flight mode name. Must be one of the modes reported by get_vehicle_status.");
        props[QStringLiteral("mode")] = mode;
        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("mode")};
        tools.append(makeFunction(QStringLiteral("set_flight_mode"),
            QStringLiteral("Set the drone's flight mode by name."), params));
    }

    tools.append(makeFunction(QStringLiteral("arm"),
        QStringLiteral("Arm the drone's motors."), noParams()));
    tools.append(makeFunction(QStringLiteral("disarm"),
        QStringLiteral("Disarm the drone's motors."), noParams()));
    tools.append(makeFunction(QStringLiteral("pause"),
        QStringLiteral("Pause the drone in place (loiter)."), noParams()));

    return tools;
}

// __EXECUTE_PLACEHOLDER__
QJsonObject AIVehicleTools::execute(const QString &name, const QJsonObject &args)
{
    if (name == QLatin1String("get_vehicle_status")) {
        return _getVehicleStatus();
    } else if (name == QLatin1String("takeoff")) {
        return _takeoff(args);
    } else if (name == QLatin1String("land")) {
        return _land();
    } else if (name == QLatin1String("return_to_launch")) {
        return _returnToLaunch();
    } else if (name == QLatin1String("goto_location")) {
        return _gotoLocation(args);
    } else if (name == QLatin1String("change_altitude")) {
        return _changeAltitude(args);
    } else if (name == QLatin1String("set_flight_mode")) {
        return _setFlightMode(args);
    } else if (name == QLatin1String("arm")) {
        return _arm(true);
    } else if (name == QLatin1String("disarm")) {
        return _arm(false);
    } else if (name == QLatin1String("pause")) {
        return _pause();
    }

    QJsonObject error;
    error[QStringLiteral("error")] = QStringLiteral("Unknown tool: ") + name;
    return error;
}

// __TOOLS_PLACEHOLDER__
QJsonObject AIVehicleTools::_getVehicleStatus()
{
    Vehicle *vehicle = _activeVehicle();
    QJsonObject status;
    if (!vehicle) {
        status[QStringLiteral("connected")] = false;
        status[QStringLiteral("message")] = QStringLiteral("No vehicle is currently connected.");
        return status;
    }

    FactGroup *vg = vehicle->vehicleFactGroup();
    FactGroup *gps = vehicle->gpsFactGroup();

    status[QStringLiteral("connected")] = true;
    status[QStringLiteral("armed")] = vehicle->armed();
    status[QStringLiteral("flight_mode")] = vehicle->flightMode();
    status[QStringLiteral("available_flight_modes")] = QJsonArray::fromStringList(vehicle->flightModes());
    status[QStringLiteral("flying")] = vehicle->flying();
    status[QStringLiteral("landing")] = vehicle->landing();
    status[QStringLiteral("ready_to_fly")] = vehicle->readyToFly();
    status[QStringLiteral("vehicle_type")] = vehicle->vehicleTypeString();

    status[QStringLiteral("altitude_relative_m")] = _factValue(vg, QStringLiteral("altitudeRelative"));
    status[QStringLiteral("altitude_amsl_m")] = _factValue(vg, QStringLiteral("altitudeAMSL"));
    status[QStringLiteral("ground_speed_ms")] = _factValue(vg, QStringLiteral("groundSpeed"));
    status[QStringLiteral("air_speed_ms")] = _factValue(vg, QStringLiteral("airSpeed"));
    status[QStringLiteral("climb_rate_ms")] = _factValue(vg, QStringLiteral("climbRate"));
    status[QStringLiteral("heading_deg")] = _factValue(vg, QStringLiteral("heading"));
    status[QStringLiteral("roll_deg")] = _factValue(vg, QStringLiteral("roll"));
    status[QStringLiteral("pitch_deg")] = _factValue(vg, QStringLiteral("pitch"));
    status[QStringLiteral("distance_to_home_m")] = _factValue(vg, QStringLiteral("distanceToHome"));
    status[QStringLiteral("throttle_pct")] = _factValue(vg, QStringLiteral("throttlePct"));

    QJsonObject gpsObj;
    gpsObj[QStringLiteral("satellites")] = _factValue(gps, QStringLiteral("count"));
    gpsObj[QStringLiteral("hdop")] = _factValue(gps, QStringLiteral("hdop"));
    Fact *lockFact = gps ? gps->getFact(QStringLiteral("lock")) : nullptr;
    gpsObj[QStringLiteral("lock")] = lockFact ? lockFact->cookedValueString() : QString();
    status[QStringLiteral("gps")] = gpsObj;

    const QGeoCoordinate coord = vehicle->coordinate();
    if (coord.isValid()) {
        QJsonObject pos;
        pos[QStringLiteral("latitude")] = coord.latitude();
        pos[QStringLiteral("longitude")] = coord.longitude();
        status[QStringLiteral("position")] = pos;
    }

    const QGeoCoordinate home = vehicle->homePosition();
    if (home.isValid()) {
        QJsonObject hp;
        hp[QStringLiteral("latitude")] = home.latitude();
        hp[QStringLiteral("longitude")] = home.longitude();
        status[QStringLiteral("home_position")] = hp;
    }

    QObject *batteryObj = vehicle->batteries() && vehicle->batteries()->count() > 0
        ? vehicle->batteries()->get(0) : nullptr;
    FactGroup *battery = qobject_cast<FactGroup *>(batteryObj);
    if (battery) {
        QJsonObject batt;
        batt[QStringLiteral("voltage_v")] = _factValue(battery, QStringLiteral("voltage"));
        batt[QStringLiteral("current_a")] = _factValue(battery, QStringLiteral("current"));
        batt[QStringLiteral("remaining_pct")] = _factValue(battery, QStringLiteral("percentRemaining"));
        batt[QStringLiteral("temperature_c")] = _factValue(battery, QStringLiteral("temperature"));
        status[QStringLiteral("battery")] = batt;
    }

    return status;
}

// __CONTROL_PLACEHOLDER__
namespace {
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

QJsonObject AIVehicleTools::_takeoff(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    const double altitude = args.value(QStringLiteral("altitude_m")).toDouble();
    if (altitude <= 0.0) {
        return errorResult(QStringLiteral("altitude_m must be greater than 0."));
    }
    vehicle->guidedModeTakeoff(altitude);
    return okResult(QStringLiteral("Takeoff command sent to %1 m.").arg(altitude));
}

QJsonObject AIVehicleTools::_land()
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    vehicle->guidedModeLand();
    return okResult(QStringLiteral("Land command sent."));
}

QJsonObject AIVehicleTools::_returnToLaunch()
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    vehicle->guidedModeRTL(false);
    return okResult(QStringLiteral("Return-to-launch command sent."));
}

QJsonObject AIVehicleTools::_gotoLocation(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    const double lat = args.value(QStringLiteral("latitude")).toDouble();
    const double lon = args.value(QStringLiteral("longitude")).toDouble();
    const QGeoCoordinate coord(lat, lon);
    if (!coord.isValid()) {
        return errorResult(QStringLiteral("Invalid latitude/longitude."));
    }
    const bool ok = vehicle->guidedModeGotoLocation(coord);
    return ok ? okResult(QStringLiteral("Goto command sent to %1, %2.").arg(lat).arg(lon))
              : errorResult(QStringLiteral("Vehicle rejected the goto command (not in a guided-capable state?)."));
}

QJsonObject AIVehicleTools::_changeAltitude(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    const double delta = args.value(QStringLiteral("delta_m")).toDouble();
    vehicle->guidedModeChangeAltitude(delta, false);
    return okResult(QStringLiteral("Altitude change command sent (%1 m).").arg(delta));
}

QJsonObject AIVehicleTools::_setFlightMode(const QJsonObject &args)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    const QString mode = args.value(QStringLiteral("mode")).toString();
    if (!vehicle->flightModes().contains(mode)) {
        return errorResult(QStringLiteral("Unknown flight mode '%1'. Available: %2")
                               .arg(mode, vehicle->flightModes().join(QStringLiteral(", "))));
    }
    vehicle->setFlightMode(mode);
    return okResult(QStringLiteral("Flight mode set to %1.").arg(mode));
}

QJsonObject AIVehicleTools::_arm(bool arm)
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    vehicle->setArmed(arm, true);
    return okResult(arm ? QStringLiteral("Arm command sent.") : QStringLiteral("Disarm command sent."));
}

QJsonObject AIVehicleTools::_pause()
{
    Vehicle *vehicle = _activeVehicle();
    if (!vehicle) {
        return errorResult(QStringLiteral("No vehicle connected."));
    }
    vehicle->pauseVehicle();
    return okResult(QStringLiteral("Pause command sent."));
}




