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
    if (!r.init({1280, 720, true, "SAM Viewer (Phase 4)"})) return 1;
    r.attachBus(&bus);  // <— attach

    double t = 0.0;
    while (!r.shouldClose()) {
        // fabricate a moving point (replace with real sim later)
        t += 0.016;
        vis::TelemetrySample s{};
        s.t  = t;
        // missile (yellow)
        s.mx = 500.f * std::cos(t);
        s.my = 500.f * std::sin(t);
        s.mz = 100.f;

        // target (cyan) — move it so it’s obviously separate
        s.tx = 800.f * std::cos(0.5*t);
        s.ty = 800.f * std::sin(0.5*t);
        s.tz = 0.f;

        bus.push(s);

        r.beginFrame();
        r.drawScene();   // now draws trails + markers
        r.endFrame();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    r.shutdown();
    return 0;
}
