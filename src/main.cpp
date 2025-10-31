#include <iostream>
#include <iomanip>
#include <typeinfo> 
#include <fstream>
#include <cxxabi.h>
#include <memory>
#include "sim/State.hpp"
#include "sim/Integrator.hpp"
#include "sim/Missile.hpp"
#include "sim/JSONFactory.hpp"

auto demangle = [](const std::type_info& ti) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
    std::string out = (status == 0 && demangled) ? demangled : ti.name();
    free(demangled);
    return out;
};

using namespace sim;

int main(int argc, char** argv) {
    std::string config_path = (argc > 1) ? argv[1] : std::string("../configs/simple_scenario.json");

    // create missile and modules
    Missile missile;
    FactoryParams params;
    std::string err;
    if (!load_from_json(config_path, missile, params, err)) {
        std::cerr << "Config error: " << err << "";
        return 1;
    }
    // ------------------------------------------------------------
    // Diagnostic: print parsed scenario contents in human-readable form
    // ------------------------------------------------------------
    std::cout << "\n=== Parsed Configuration Summary ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Config file: " << config_path << "\n";
    std::cout << "Simulation parameters:\n";
    std::cout << "  t0 = " << params.t0
            << ", tf = " << params.tf
            << ", h = " << params.h << "\n";

    auto print_state = [](const State& s, const std::string& label) {
        if (s.size() >= 6) {
            std::cout << "  " << label << ":\n"
                    << "    pos = [" << s.x[0] << ", " << s.x[1] << ", " << s.x[2] << "]\n"
                    << "    vel = [" << s.x[3] << ", " << s.x[4] << ", " << s.x[5] << "]\n";
        } else {
            std::cout << "  " << label << " has size " << s.size() << " (expected 6)\n";
        }
    };

    std::cout << "\nInitial states:\n";
    print_state(params.missile0, "Missile0");
    print_state(params.target0,  "Target0");

    std::cout << "\nModules constructed:\n";
    std::cout << "  Aero:      " << (missile.aero     ? demangle(typeid(*missile.aero))      : "<null>") << "\n";
    std::cout << "  Guidance:  " << (missile.guidance ? demangle(typeid(*missile.guidance))  : "<null>") << "\n";
    std::cout << "  Autopilot: " << (missile.autopilot? demangle(typeid(*missile.autopilot)) : "<null>") << "\n";

    std::cout << "====================================\n\n";


    // Initialize states
    missile.state = params.missile0;
    missile.state.t = params.t0;
    State target = params.target0; // position + velocity

    // simulation parameters
    double t = params.t0;
    double tf = params.tf;
    double h  = params.h;
    double t_target = t;

    std::ofstream out("telemetry.csv");
    out << "t,px,py,pz,vx,vy,vz,tx,ty,tz\n";

    State s = missile.state;

    while (t < tf) {
        // 1) Update target to current sim time t (BEFORE guidance)
        double dt_target = t - t_target;
        if (dt_target != 0.0) {
            target.x[0] += target.x[3] * dt_target;  // x += vx * dt
            target.x[1] += target.x[4] * dt_target;  // y += vy * dt
            target.x[2] += target.x[5] * dt_target;  // z += vz * dt
            t_target = t;
        }
        // 2) Compute guidance using the UPDATED target position at time t
        auto guidanceCmd = missile.guidance->guidanceCommand(s, target);

        // 3) Autopilot
        auto control = missile.autopilot->control(s, guidanceCmd);

        // 4) Integrate missile dynamics
        DerivFunc f = [&](double tt, const State& yy) {
            (void)tt; // unused here
            return missile.derivative(tt, yy, control);
        };
        State snew = RK4Integrator::step(f, t, s, h);
        snew.t = t + h;

        // write telemetry (missile + target)
        out << snew.t << ","
            << snew.x[0] << "," << snew.x[1] << "," << snew.x[2] << ","
            << snew.x[3] << "," << snew.x[4] << "," << snew.x[5] << ","
            << target.x[0] << "," << target.x[1] << "," << target.x[2] << "\n";

        s = snew;
        t += h;

        // naive termination is near target
        double dx = s.x[0] - target.x[0];
        double dy = s.x[1] - target.x[1];
        double dz = s.x[2] - target.x[2];
        double dist2 = dx*dx + dy*dy + dz*dz;
        if (dist2 <= 1.0) {
            std::cout << "Intercept (approx) at t=" << t << "\n";
            break;
        }
    }

    out.close();
    std::cout << "Simulation finished, telemetry.csv written." << std::endl;
    return 0;
}