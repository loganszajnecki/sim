// Application.hpp
#pragma once

#include <string>

#include "app/ScenarioLoader.hpp"
#include "app/SimulationEngine.hpp"
#include "app/LoopController.hpp"
#include "app/TelemetryCsvWriter.hpp"

#ifdef APP_WITH_VIEWER
  #include "app/RendererController.hpp"
#endif

namespace app {

/**
 * @brief Runtime configuration options for the Application.
 *
 * These are typically set at startup (e.g., from command line flags).
 */
struct ApplicationOptions {
    /// Path to the JSON scenario configuration file.
    std::string config_path{"../configs/simple_scenario.json"};

    /// If true, run without creating a viewer window (fast, headless mode).
    /// Note: For the headless binary (sam_sim_headless), this is always true.
    bool headless{false};

    /// Maximum number of physics steps to take per rendered frame
    /// in interactive mode (used by LoopController).
    int maxStepsPerFrame{10};

    /// Intercept radius [m] for declaring a hit.
    double intercept_radius_m{500.0};
};

/**
 * @brief Top-level application class that wires together:
 *
 *  - Scenario loading (JSON → sim objects)
 *  - Simulation engine (Missile, target, integrator, intercept logic)
 *  - Telemetry logging to CSV
 *  - Optional OpenGL viewer (when compiled with APP_WITH_VIEWER)
 *
 * The same Application code is compiled in two configurations:
 *
 *  - sam_sim_headless: built without APP_WITH_VIEWER, always runs headless.
 *  - sam_sim: built with APP_WITH_VIEWER=1, runs with viewer unless
 *             opts_.headless is true or viewer initialization fails.
 */
class Application {
public:
    explicit Application(ApplicationOptions opts);

    /// Run the application. Returns 0 on success, non-zero on error.
    int run();

private:
    /// Headless "fast" mode: no viewer, run as fast as possible.
    int runHeadless();

#ifdef APP_WITH_VIEWER
    /// Interactive mode: run sim + viewer with real-time rendering.
    int runInteractive();
#endif

    // -------------------------------------------------------------------------
    // Configuration and state
    // -------------------------------------------------------------------------
    ApplicationOptions opts_;
    Scenario           scenario_;

    // Core simulation engine; constructed with dummy values and then
    // reconfigured once the scenario is loaded.
    SimulationEngine   engine_{sim::Missile{}, sim::State{}, 5.0};

    // Controls how the sim advances relative to real time (interactive mode).
    LoopController     loop_{0.01, 10};

    // Telemetry writer; the flush behavior is finalized in the constructor.
    TelemetryCsvWriter csv_{"telemetry.csv", true};

#ifdef APP_WITH_VIEWER
    // Viewer/controller wrapper for the OpenGL renderer.
    RendererController viewer_;
#endif

    // Time bookkeeping
    double t_{0.0};
    double tf_{0.0};
    bool   sim_done_{false};
    double last_csv_t_{0.0};
};

} // namespace app
