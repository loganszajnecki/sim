#include "sim/JSONFactory.hpp"
#include "modules/SimpleAero.hpp"
#include "modules/PDAutopilot.hpp"
#include "modules/ProNav.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace sim {

using nlohmann::json;

static bool read_json_file(const std::string& path, json& j, std::string& err)
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

static double get_or(const json& j, const char* key, double def)
{
    return j.contains(key) ? j.at(key).get<double>() : def;
}

bool load_from_json(const std::string &path, Missile &missile, FactoryParams &out, std::string &error)
{
    json j;
    if (!read_json_file(path, j, error)) return false;

    // timings
    out.t0 = j.value("t0", 0.0);
    out.tf = j.value("tf", 60.0);
    out.h  = j.value("h", 0.01);

    // initial missile state
    out.missile0 = State(6);
    if (j.contains("initial_state")) {
        auto is = j["initial_state"];
        auto pos = is.value("pos", std::vector<double>{0,0,0});
        auto vel = is.value("vel", std::vector<double>{0,0,0});
        for (int i = 0; i < 3 && i < (int)pos.size(); ++i) {
            out.missile0.x[i] = pos[i];
        }
        for (int i = 0; i < 3 && i < (int)vel.size(); ++i) {
            out.missile0.x[3+i] = vel[i];
        }
    }
    // target state
    out.target0 = State(6);
    if (j.contains("target")) {
        auto is = j["target"];
        auto pos = is.value("pos", std::vector<double>{0,0,0});
        auto vel = is.value("vel", std::vector<double>{0,0,0});
        for (int i = 0; i < 3 && i < (int)pos.size(); ++i) {
            out.target0.x[i] = pos[i];
        }
        for (int i = 0; i < 3 && i < (int)vel.size(); ++i) {
            out.target0.x[3+i] = vel[i];
        }
    }

    // modules
    // Aero
    if (j.contains("missile") && j["missile"].contains("aero")) {
        auto a = j["missile"]["aero"];
        std::string name = a.value("type", std::string("SimpleAero"));
        if (name == "SimpleAero") {
            double gain = get_or(a, "gain", 1.0);
            double damping = get_or(a, "damping", 0.0);
            double thrust = get_or(a, "thrustAcc", 0.0);
            missile.aero = std::make_unique<modules::SimpleAero>(gain, damping, thrust);
        } else {
            error = "Unknown aero type: " + name;
            return false;
        }
    } else {
        missile.aero = std::make_unique<modules::SimpleAero>(1.0, 0.0, 0.0);
    }

    // Guidance
    if (j.contains("missile") && j["missile"].contains("guidance")) {
        auto g = j["missile"]["guidance"];
        std::string name = g.value("type", std::string("ProNav"));
        if (name == "ProNav") {
            double N = get_or(g, "N", 3.5);
            double amax = get_or(g, "amax", 50.0);
            missile.guidance = std::make_unique<modules::ProNav>(N, amax);
        } else {
            error = "Unknown guidance type: " + name;
            return false;
        }
    }

    // Autopilot
    if (j.contains("missile") && j["missile"].contains("autopilot")) {
        auto ap = j["missile"]["autopilot"];
        std::string name = ap.value("type", std::string("PD"));
        if (name == "PD") {
            double kp = get_or(ap, "kp", 1.0);
            double kd = get_or(ap, "kd", 0.0);
            missile.autopilot = std::make_unique<modules::PDAutopilot>(kp, kd);
        } else {
            error = "Unknown autopilot type: " + name;
            return false;
        }
    }

    return true;
}
} // namespace sim