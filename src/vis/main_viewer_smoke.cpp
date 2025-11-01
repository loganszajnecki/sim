// src/vis/main_viewer_smoke.cpp
#include "vis/Renderer.hpp"
#include "vis/Telemetry.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstdio>

int main() {
    vis::TelemetryBus bus;
    vis::Renderer r;
    if (!r.init({1280, 720, true, "SAM Viewer (Phase 3 - Bus Test)"})) return 1;

    std::vector<vis::TelemetrySample> drained;
    drained.reserve(1<<16);

    double t = 0.0;
    while (!r.shouldClose()) {
        // ---- fabricate a moving point (remove in Phase 4) ----
        t += 0.016;
        vis::TelemetrySample s{};
        s.t  = t;
        s.mx = 500.f * std::cos(t);
        s.my = 500.f * std::sin(t);
        s.mz = 100.f;
        bus.push(s);

        // ---- drain whatever arrived (for now, just keep/count) ----
        size_t got = bus.drain(drained, 16384);
        if (got) {
            // printf every ~1s to avoid spam
            static double t_last = 0;
            if (t - t_last > 1.0) {
                const auto& last = drained.back();
                std::printf("[viewer] drained=%zu  last(mx,my,mz)=(%.1f,%.1f,%.1f)\n",
                            drained.size(), last.mx, last.my, last.mz);
                t_last = t;
            }
        }

        r.beginFrame();
        r.drawScene();     // just grid/axes for Phase 3
        r.endFrame();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    r.shutdown();
    return 0;
}
