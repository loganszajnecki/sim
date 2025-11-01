#pragma once
#include "vis/Renderer.hpp"
#include "vis/Telemetry.hpp"

namespace app {

struct ViewerSettings {
    int width = 1920;
    int height = 1080;
    bool vsync = true;
    const char* title = "SAM Viewer (Live)";
};

class RendererController {
public:
    RendererController() {
        bus_.set_max(200'000);
    }

    // Returns true if initialized and attached to the bus.
    bool init(const ViewerSettings& s) {
        if (initialized_) return true; // idempotent
        if (!renderer_.init({s.width, s.height, s.vsync, s.title})) {
            return false;
        }
        renderer_.attachBus(&bus_);
        initialized_ = true;
        return true;
    }

    // Safe destructor: shuts down if needed, guards repeated calls.
    ~RendererController() {
        safeShutdown();
    }

    // Non-copyable, allow move if you want later
    RendererController(const RendererController&) = delete;
    RendererController& operator=(const RendererController&) = delete;

    bool enabled() const { return initialized_; }

    bool shouldClose() const {
        return initialized_ && renderer_.shouldClose();
    }

    void push(const vis::TelemetrySample& ts) {
        if (initialized_) bus_.push(ts);
    }

    void renderFrame() {
        if (!initialized_) return;
        renderer_.beginFrame();
        renderer_.drawScene();
        renderer_.endFrame();
    }

    // Optional explicit shutdown (not required by caller)
    void shutdown() { safeShutdown(); }

    // Expose bus if you need it later (e.g., for HUD layers)
    vis::TelemetryBus& bus() { return bus_; }

private:
    void safeShutdown() {
        if (initialized_) {
            // Guard against double shutdown / tear-down ordering
            // Make sure nothing uses the bus after this point.
            renderer_.shutdown();
            initialized_ = false;
        }
    }

    bool initialized_ = false;
    vis::TelemetryBus bus_;
    vis::Renderer renderer_;
};

} // namespace app
