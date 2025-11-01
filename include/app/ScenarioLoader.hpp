#pragma once
#include "sim/Missile.hpp"
#include "sim/JSONFactory.hpp"
#include <string>
#include <tuple>
#include <typeinfo>
#include <cxxabi.h>
#include <iostream>
#include <iomanip>

namespace app {

struct Scenario {
    sim::Missile  missile;
    sim::FactoryParams params;
};

// Load scenario; return (ok, Scenario, err)
inline std::tuple<bool, Scenario, std::string>
loadScenario(const std::string& path) {
    Scenario sc{};
    std::string err;
    sim::FactoryParams p{};

    if (!load_from_json(path, sc.missile, p, err)) {
        // OK: Scenario{} is a temporary (movable), err is copied
        return std::make_tuple(false, Scenario{}, err);
    }
    sc.params = p;
    return std::make_tuple(true, std::move(sc), std::string{});
}

// Optional: pretty diagnostics you were printing in main
inline void printScenarioSummary(const std::string& config_path,
                                 const Scenario& sc) {
    auto demangle = [](const std::type_info& ti) {
        int status = 0;
        char* demangled = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
        std::string out = (status == 0 && demangled) ? demangled : ti.name();
        free(demangled);
        return out;
    };
    using sim::State;

    auto print_state = [](const State& s, const std::string& label) {
        std::cout << "  " << label << ":\n"
                  << "    pos = [" << s.x[0] << ", " << s.x[1] << ", " << s.x[2] << "]\n"
                  << "    vel = [" << s.x[3] << ", " << s.x[4] << ", " << s.x[5] << "]\n";
    };

    const auto& P = sc.params;
    const auto& M = sc.missile;

    std::cout << "\n=== Parsed Configuration Summary ===\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Config file: " << config_path << "\n";
    std::cout << "Simulation parameters:\n";
    std::cout << "  t0 = " << P.t0 << ", tf = " << P.tf << ", h = " << P.h << "\n";
    std::cout << "\nInitial states:\n";
    print_state(P.missile0, "Missile0");
    print_state(P.target0,  "Target0");

    std::cout << "\nModules constructed:\n";
    std::cout << "  Aero:      " << (M.aero      ? demangle(typeid(*M.aero))      : "<null>") << "\n";
    std::cout << "  Guidance:  " << (M.guidance  ? demangle(typeid(*M.guidance))  : "<null>") << "\n";
    std::cout << "  Autopilot: " << (M.autopilot ? demangle(typeid(*M.autopilot)) : "<null>") << "\n";
    std::cout << "====================================\n\n";
}

} // namespace app
