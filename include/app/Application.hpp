#pragma once
#include <string>
#include <utility>

#include "app/ScenarioLoader.hpp"
#include "app/SimulationEngine.hpp"
#include "app/LoopController.hpp"
#include "app/TelemetryCsvWriter.hpp"

#ifdef APP_WITH_VIEWER
  #include "app/RendererController.hpp"
#endif

namespace app {

struct ApplicationOptions {
    std::string config_path = "../configs/simple_scenario.json";
    bool headless = false;
    int  maxStepsPerFrame = 10;
    double intercept_radius_m = 5.0;
};

class Application {
public:
    explicit Application(ApplicationOptions opts);
    int run();

private:
    int runHeadless();
#ifdef APP_WITH_VIEWER
    int runInteractive();
#endif

    ApplicationOptions  opts_;
    Scenario            scenario_;
    SimulationEngine    engine_{sim::Missile{}, sim::State{}, 5.0};
    LoopController      loop_{0.01, 10};
    TelemetryCsvWriter  csv_{"telemetry.csv", true};

#ifdef APP_WITH_VIEWER
    RendererController  viewer_;
#endif

    double t_  = 0.0;
    double tf_ = 0.0;
    bool   sim_done_ = false;
    double last_csv_t_ = 0.0;
};

} // namespace app
