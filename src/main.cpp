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
#include <GLFW/glfw3.h>

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

    vis::TelemetryBus bus;
    bus.set_max(200000);
    bool viewer_enabled = true;
    vis::Renderer renderer;
    if (viewer_enabled) {
        if (!renderer.init({1280, 720, true, "SAM Viewer (Live)"})) {
            std::cerr << "Viewer init failed; running headless.\n";
            viewer_enabled = false;
        } else {
            renderer.attachBus(&bus);
        }
    }
    // Initialize states
    missile.state = params.missile0;
    missile.state.t = params.t0;
    State target = params.target0; // position + velocity

    // simulation parameters
    double t = params.t0;
    double tf = params.tf;
    double h  = params.h;
    double t_target = t;

    // timing for render/sim catch-up
    double lastWall = glfwGetTime();
    double simLag   = 0.0;              // how much real time the sim is behind

    std::ofstream out("telemetry.csv");
    out << "t,px,py,pz,vx,vy,vz,tx,ty,tz\n";

    State s = missile.state;

    while (t < tf) {
        // Handle window close
        if (viewer_enabled && renderer.shouldClose()) break;

        // wall-clock delta
        double now = glfwGetTime();
        double frameDt = now - lastWall;
        lastWall = now;
        simLag += frameDt;

        // Step the sim in fixed increments (h) to catch up with real-time
        int maxStepsPerFrame = 10; // safety to avoid spiral of death
        int steps = 0;
        while (simLag >= h && steps < maxStepsPerFrame && t < tf) {

            // 1) Update target kinematics to time t
            double dt_target = h;               // we advance exactly h each substep
            target.x[0] += target.x[3] * dt_target;
            target.x[1] += target.x[4] * dt_target;
            target.x[2] += target.x[5] * dt_target;
            t_target = t + h;

            // 2) Guidance
            auto guidanceCmd = missile.guidance->guidanceCommand(s, target);

            // 3) Autopilot
            auto control = missile.autopilot->control(s, guidanceCmd);

            // 4) Integrate dynamics (RK4)
            DerivFunc f = [&](double /*tt*/, const State& yy) {
                return missile.derivative(t, yy, control);
            };
            State snew = RK4Integrator::step(f, t, s, h);
            snew.t = t + h;

            // ---- Push telemetry sample for viewer ----
            vis::TelemetrySample ts{};
            ts.t  = snew.t;
            ts.mx = (float)snew.x[0]; ts.my = (float)snew.x[1]; ts.mz = (float)snew.x[2];
            ts.mvx= (float)snew.x[3]; ts.mvy= (float)snew.x[4]; ts.mvz= (float)snew.x[5];
            ts.tx = (float)target.x[0]; ts.ty = (float)target.x[1]; ts.tz = (float)target.x[2];
            // if available:
            // ts.ax = (float)guidanceCmd.ax; ts.ay = (float)guidanceCmd.ay; ts.az = (float)guidanceCmd.az;
            bus.push(ts);

            // CSV (unchanged)
            out << snew.t << ","
                << snew.x[0] << "," << snew.x[1] << "," << snew.x[2] << ","
                << snew.x[3] << "," << snew.x[4] << "," << snew.x[5] << ","
                << target.x[0] << "," << target.x[1] << "," << target.x[2] << "\n";

            // commit substep
            s = snew;
            t += h;
            simLag -= h;
            ++steps;

            // naive termination
            double dx = s.x[0] - target.x[0];
            double dy = s.x[1] - target.x[1];
            double dz = s.x[2] - target.x[2];
            if (dx*dx + dy*dy + dz*dz <= 1.0) {
                std::cout << "Intercept (approx) at t=" << t << "\n";
                simLag = 0.0;
                break;
            }
        }

        // Render one frame
        if (viewer_enabled) {
            renderer.beginFrame();
            renderer.drawScene();   // consumes bus, draws trails + markers
            renderer.endFrame();
        }

        // Optional tiny sleep to avoid 100% CPU if headless or sim is ahead
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    out.close();
    if (viewer_enabled) renderer.shutdown();
    std::cout << "Simulation finished, telemetry.csv written.\n";
    return 0;
}