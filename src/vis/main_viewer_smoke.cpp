#include "vis/Renderer.hpp"
#include <thread>
#include <chrono>

int main() {
    vis::Renderer r;
    if (!r.init({1920, 1080, true, "SAM Viewer (Phase 1)" })) return 1;

    while (!r.shouldClose()) {
        r.beginFrame();
        r.drawScene();
        r.endFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // keep CPU calm
    }
    r.shutdown();
    return 0;
}
