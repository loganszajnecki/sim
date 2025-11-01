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
#include "vis/Telemetry.hpp"
#include "vis/Renderer.hpp"
#include "app/TelemetryCsvWriter.hpp"
#include "app/RendererController.hpp"
#include "app/SimulationEngine.hpp"

#include <GLFW/glfw3.h>

auto demangle = [](const std::type_info& ti) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
    std::string out = (status == 0 && demangled) ? demangled : ti.name();
    free(demangled);
    return out;
};

int main(int argc, char** argv) {
    using namespace sim;

    // Config path
    std::string config_path = (argc > 1) ? argv[1] : std::string("../configs/simple_scenario.json");

    // CSV (line-flushed writer you added in Phase 1)
    app::TelemetryCsvWriter csv("telemetry.csv");
    if (!csv.good()) {
        std::cerr << "Warning: failed to open telemetry.csv for writing\n";
    }

    // Load scenario (missile + params)
    Missile missile;
    FactoryParams params;
    std::string err;
    if (!load_from_json(config_path, missile, params, err)) {
        std::cerr << "Config error: " << err << "\n";
        return 1;
    }

    // Diagnostics
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

    app::RendererController viewer;
    bool viewer_enabled = viewer.init({1920, 1080, true, "SAM Viewer (Live)"});
    if (!viewer_enabled) {
        std::cerr << "Viewer init failed; running headless.\n";
    }

    app::SimulationEngine engine(std::move(missile), params.target0, /*intercept r (m)*/ 5.0);
    engine.setInitial(params.missile0, params.t0);


    // Initialize states
    missile.state = params.missile0;
    missile.state.t = params.t0;
    State target = params.target0; // position + velocity

    // Time management
    double t  = params.t0;
    double tf = params.tf;
    double h  = params.h;

    double lastWall = glfwGetTime();
    double simLag   = 0.0;

    bool sim_done = false;
    while (t < tf) {
        // Allow user to close the window at any time
        if (viewer_enabled && viewer.shouldClose()) break;

        // accumulate real time
        double now     = glfwGetTime();
        double frameDt = now - lastWall;
        lastWall       = now;
        simLag        += frameDt;
        const int maxStepsPerFrame = 10;
        int steps = 0;

        if (!sim_done) {
            // Step the sim in fixed increments (h) to catch up with real-time
            while (simLag >= h && steps < maxStepsPerFrame && t < tf) {
                auto hit = engine.step(h);

                // Push telemetry to viewer
                if (viewer_enabled) {
                    const auto& ms = engine.missile();
                    const auto& tg = engine.target();
                    vis::TelemetrySample ts{};
                    ts.t  = (float)ms.t;
                    ts.mx = (float)ms.x[0]; ts.my = (float)ms.x[1]; ts.mz = (float)ms.x[2];
                    ts.mvx= (float)ms.x[3]; ts.mvy= (float)ms.x[4]; ts.mvz= (float)ms.x[5];
                    ts.tx = (float)tg.x[0]; ts.ty = (float)tg.x[1]; ts.tz = (float)tg.x[2];
                    viewer.push(ts);
                }
                
                // CSV
                csv.write(engine.missile().t, engine.missile(), engine.target());

                // commit substep time
                t      += h;
                simLag -= h;
                ++steps;

                // intercept?
                if (hit) {
                    std::cout << "Intercept (approx) at t = " << hit->t << " s\n";
                    std::cout << "POCA = " << hit->poca << " m\n";
                    csv.flush();      // ensures final line is on disk even if something crashes later
                    sim_done = true;
                    simLag   = 0.0;
                    break;
                }
            }
        } else {
            // Physics is done; no more stepping. Keep rendering until user closes window.
            simLag = 0.0;
        }

        // Render one frame (always render, even after intercept)
        if (viewer_enabled) {
            viewer.renderFrame();
        }

        // If running headless and physics is done, exit outer loop
        if (sim_done && !viewer_enabled) break;
    }

    std::cout << "Simulation finished, telemetry.csv written.\n";
    return 0;
}