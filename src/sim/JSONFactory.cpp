#include "sim/JSONFactory.hpp"

#include "sim/models/SimpleAero.hpp"
#include "sim/models/SimpleAccelAutopilot.hpp"
#include "sim/models/ProNav.hpp"

#include "geo/GeoTypes.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace sim {

using nlohmann::json;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

namespace {

// Read JSON file from disk into a nlohmann::json object.
bool read_json_file(const std::string& path, json& j, std::string& err)
{
    std::ifstream f(path);
    if (!f) {
        err = "Cannot open config file: " + path + "\n";
        return false;
    }

    try {
        f >> j;
    } catch (const std::exception& e) {
        err = std::string("JSON parse error: ") + e.what();
        return false;
    }

    return true;
}

// Convenience: get double value or default.
double get_or(const json& j, const char* key, double def)
{
    return j.contains(key) ? j.at(key).get<double>() : def;
}

} // anonymous namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool load_from_json(const std::string& path,
                    Missile& missile,
                    FactoryParams& out,
                    std::string& error)
{
    json j;
    if (!read_json_file(path, j, error)) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Timing parameters.
    // -------------------------------------------------------------------------
    out.t0 = j.value("t0", 0.0);
    out.tf = j.value("tf", 60.0);
    out.h  = j.value("h",  0.01);

    // -------------------------------------------------------------------------
    // Geo origin (Earth-anchored frame).
    // -------------------------------------------------------------------------
    {
        geo::GeoLLA originLLA;
        if (j.contains("geo_origin")) {
            const auto& go = j["geo_origin"];
            originLLA.lat_deg = go.value("lat_deg", 0.0);
            originLLA.lon_deg = go.value("lon_deg", 0.0);
            originLLA.alt_m   = go.value("alt_m",   0.0);
        } else {
            originLLA.lat_deg = 0.0;
            originLLA.lon_deg = 0.0;
            originLLA.alt_m   = 0.0;
        }

        out.origin = geo::makeOrigin(originLLA);
    }

    // Ensure initial states are correctly sized for the current model.
    out.missile0 = State(Missile::Indices::Size);
    out.target0  = State(Missile::Indices::Size);

    // -------------------------------------------------------------------------
    // Initial missile state (in local ENU coordinates).
    // -------------------------------------------------------------------------
    if (j.contains("initial_state")) {
        const auto& is = j["initial_state"];

        const auto pos = is.value("pos", std::vector<double>{0.0, 0.0, 0.0});
        const auto vel = is.value("vel", std::vector<double>{0.0, 0.0, 0.0});

        for (std::size_t i = 0; i < 3 && i < pos.size(); ++i) {
            out.missile0.x[i] = pos[i];
        }
        for (std::size_t i = 0; i < 3 && i < vel.size(); ++i) {
            out.missile0.x[3 + i] = vel[i];
        }
    }

    // -------------------------------------------------------------------------
    // Initial target state (also in local ENU).
    // -------------------------------------------------------------------------
    if (j.contains("target")) {
        const auto& ts = j["target"];

        const auto pos = ts.value("pos", std::vector<double>{0.0, 0.0, 0.0});
        const auto vel = ts.value("vel", std::vector<double>{0.0, 0.0, 0.0});

        for (std::size_t i = 0; i < 3 && i < pos.size(); ++i) {
            out.target0.x[i] = pos[i];
        }
        for (std::size_t i = 0; i < 3 && i < vel.size(); ++i) {
            out.target0.x[3 + i] = vel[i];
        }
    }

    // -------------------------------------------------------------------------
    // Models: missile.aero / missile.guidance / missile.autopilot
    // -------------------------------------------------------------------------

    // Look up "missile" subtree once.
    const bool hasMissile = j.contains("missile");
    const json& missileCfg = hasMissile ? j["missile"] : json::object();

    // Aero
    if (hasMissile && missileCfg.contains("aero")) {
        const auto& a = missileCfg["aero"];
        std::string name = a.value("type", std::string("SimpleAero"));

        if (name == "SimpleAero") {
            const double accelGain   = get_or(a, "accelGain",   1.0);
            const double velDamping  = get_or(a, "velDamping",  0.0);
            const double thrustAccel = get_or(a, "thrustAccel", 0.0);

            missile.aero = std::make_unique<models::SimpleAero>(
                accelGain,
                velDamping,
                thrustAccel
                // gravityAccel uses default in SimpleAero
            );
        } else {
            error = "Unknown aero type: " + name;
            return false;
        }
    } else {
        // Default aero model if none specified.
        missile.aero = std::make_unique<models::SimpleAero>(1.0, 0.0, 0.0);
    }

    // Guidance
    if (hasMissile && missileCfg.contains("guidance")) {
        const auto& g = missileCfg["guidance"];
        std::string name = g.value("type", std::string("ProNav"));

        if (name == "ProNav") {
            const double N    = get_or(g, "N",    3.5);
            const double amax = get_or(g, "amax", 50.0);

            missile.guidance = std::make_unique<models::ProNav>(N, amax);
        } else {
            error = "Unknown guidance type: " + name;
            return false;
        }
    }
    // Note: If no guidance block is provided, guidance remains null and it is
    // the caller's responsibility to set a default if desired.

    // Autopilot
    if (hasMissile && missileCfg.contains("autopilot")) {
        const auto& ap = missileCfg["autopilot"];

        // Default to simple placeholder autopilot.
        std::string name = ap.value("type", std::string("SimpleAccel"));

        if (name == "SimpleAccel") {
            const double gain   = get_or(ap, "gain",   1.0);
            const double maxCmd = get_or(ap, "maxCmd", 100.0);

            missile.autopilot = std::make_unique<models::SimpleAccelAutopilot>(
                gain,
                maxCmd
            );
        } else {
            error = "Unknown autopilot type: " + name;
            return false;
        }
    }
    // Note: If no autopilot block is provided, autopilot remains null and it is
    // the caller's responsibility to set a default if desired.

    return true;
}

} // namespace sim
