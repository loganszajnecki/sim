#include "app/Application.hpp"
#include <iostream>
#include <utility>
#include <stdexcept>

namespace app {

Application::Application(ApplicationOptions opts) 
    : opts_(std::move(opts)), csv_("telemetry.csv", /*line_flush=*/ !opts_.headless) 
{
    auto [ok, sc, err] = loadScenario(opts_.config_path);
    if (!ok) throw std::runtime_error("Config error: " + err);
    scenario_ = std::move(sc);

    printScenarioSummary(opts_.config_path, scenario_);

    engine_ = SimulationEngine(std::move(scenario_.missile), scenario_.params.target0, opts_.intercept_radius_m);
    engine_.setInitial(scenario_.params.missile0, scenario_.params.t0);

    t_  = scenario_.params.t0;
    tf_ = scenario_.params.tf;
    loop_.setStep(scenario_.params.h);
    loop_.setMaxStepsPerFrame(opts_.maxStepsPerFrame);

    last_csv_t_ = t_;
}

int Application::run() {
    try {
        if (opts_.headless) return runHeadless();
#ifdef APP_WITH_VIEWER
        bool viewer_enabled = viewer_.init({1920, 1080, true, "SAM Viewer (Live)"});
        if (!viewer_enabled) {
            std::cerr << "Viewer init failed; falling back to headless fast mode.\n";
            opts_.headless = true;
            return runHeadless();
        }
        return runInteractive();
#else
        // Built without viewer: always headless fast
        opts_.headless = true;
        return runHeadless();
#endif
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}

int Application::runHeadless() {
    std::cout << "[headless] Viewer disabled; running as fast as possible\n";
    const double h = scenario_.params.h;
    std::ios::sync_with_stdio(false);
    size_t lines_since_flush = 0, FLUSH_EVERY = 2000;

    while (t_ < tf_ && !sim_done_) {
        auto hit = engine_.step(h);
        csv_.write(engine_.missile().t, engine_.missile(), engine_.target());
        last_csv_t_ = engine_.missile().t;
        t_ += h;

        if (++lines_since_flush >= FLUSH_EVERY) { csv_.flush(); lines_since_flush = 0; }
        if (hit) {
            std::cout << "Intercept (approx) at t = " << hit->t << " s\n";
            std::cout << "POCA = " << hit->poca << " m\n";
            sim_done_ = true;
        }
    }
    csv_.flush();
    std::cout << "Simulation finished, telemetry.csv written.\n";
    std::cout << "Last CSV time written: " << last_csv_t_ << " s\n";
    return 0;
}

#ifdef APP_WITH_VIEWER
int Application::runInteractive() {
    for (;;) {  // <-- run until user closes the window
        if (viewer_.shouldClose()) break;

        auto plan = loop_.beginFrame();

        // Advance physics only while the sim isn't finished
        if (!sim_done_ && t_ < tf_) {
            for (int i = 0; i < plan.steps && t_ < tf_; ++i) {
                auto hit = engine_.step(plan.h);

                // CSV (no more writes once sim_done_ goes true)
                csv_.write(engine_.missile().t, engine_.missile(), engine_.target());
                last_csv_t_ = engine_.missile().t;

                // Push latest sample to viewer
                const auto& ms = engine_.missile();
                const auto& tg = engine_.target();
                vis::TelemetrySample ts{};
                ts.t  = (float)ms.t;
                ts.mx = (float)ms.x[0]; ts.my=(float)ms.x[1]; ts.mz=(float)ms.x[2];
                ts.mvx= (float)ms.x[3]; ts.mvy=(float)ms.x[4]; ts.mvz=(float)ms.x[5];
                ts.tx = (float)tg.x[0]; ts.ty=(float)tg.x[1]; ts.tz=(float)tg.x[2];
                viewer_.push(ts);

                t_ += plan.h;

                if (hit) {
                    std::cout << "Intercept (approx) at t = " << hit->t << " s\n";
                    std::cout << "POCA = " << hit->poca << " m\n";
                    csv_.flush();
                    sim_done_ = true;
                    loop_.resetLag(); // stop any further catch-up
                    break;
                }
            }
        } else {
            // No more physics; ensure accumulator doesn't pile up
            loop_.resetLag();
        }

        // Always render a frame (even after sim finished)
        viewer_.renderFrame();
    }

    std::cout << "Simulation finished, telemetry.csv written.\n";
    std::cout << "Last CSV time written: " << last_csv_t_ << " s\n";
    return 0;
}
#endif

} // namespace app
