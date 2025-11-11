#pragma once

#include "vis/Renderer.hpp"
#include "vis/Telemetry.hpp"

namespace app {

/**
 * @brief Configuration for the viewer window.
 */
struct ViewerSettings {
    int  width  = 1920;
    int  height = 1080;
    bool vsync  = true;
    const char* title = "SAM Viewer (Live)";
};

/**
 * @brief RAII wrapper around vis::Renderer + TelemetryBus.
 *
 * Responsibilities:
 *  - Owns a vis::Renderer and a TelemetryBus.
 *  - Initializes the renderer and attaches the bus.
 *  - Ensures renderer shutdown on destruction (safe, idempotent).
 *
 * Typical usage:
 *   RendererController viewer;
 *   if (viewer.init(settings)) {
 *       while (!viewer.shouldClose()) {
 *           // push samples from sim
 *           viewer.renderFrame();
 *       }
 *   }
 */
class RendererController {
public:
    RendererController()
    {
        // Set a reasonable upper bound on buffered telemetry samples.
        bus_.set_max(200'000);
    }

    /// Safe destructor: shuts down the renderer if it was initialized.
    ~RendererController() noexcept
    {
        safeShutdown();
    }

    RendererController(const RendererController&) = delete;
    RendererController& operator=(const RendererController&) = delete;

    // (Move operations are intentionally not provided; if needed later,
    //  they can be explicitly implemented once vis::Renderer/TelemetryBus
    //  semantics are clear.)

    /**
     * @brief Initialize the renderer and attach the telemetry bus.
     *
     * This function is idempotent: calling init() multiple times after
     * a successful initialization is a no-op and returns true.
     *
     * @return true if initialization succeeded or was already done,
     *         false if renderer initialization failed.
     */
    [[nodiscard]] bool init(const ViewerSettings& s)
    {
        if (initialized_) {
            return true; // idempotent
        }

        if (!renderer_.init({s.width, s.height, s.vsync, s.title})) {
            return false;
        }

        renderer_.attachBus(&bus_);
        initialized_ = true;
        return true;
    }

    /// @return true if the renderer has been successfully initialized.
    [[nodiscard]] bool enabled() const noexcept { return initialized_; }

    /// @return true if a window close has been requested (or not initialized).
    [[nodiscard]] bool shouldClose() const
    {
        return initialized_ && renderer_.shouldClose();
    }

    /// Push a telemetry sample into the bus (no-op if not initialized).
    void push(const vis::TelemetrySample& ts)
    {
        if (initialized_) {
            bus_.push(ts);
        }
    }

    /// Render a single frame (no-op if not initialized).
    void renderFrame()
    {
        if (!initialized_) {
            return;
        }
        if (!renderer_.beginFrame()) {
            return;
        }
        renderer_.drawScene();
        renderer_.endFrame();
    }

    /// Optional explicit shutdown (not required; destructor already calls this).
    void shutdown() { safeShutdown(); }

    /// Access to the underlying telemetry bus (e.g. for HUD overlays).
    vis::TelemetryBus& bus() { return bus_; }

private:
    void safeShutdown()
    {
        if (initialized_) {
            // Guard against double shutdown / teardown ordering issues.
            renderer_.shutdown();
            initialized_ = false;
        }
    }

    bool             initialized_{false};
    vis::TelemetryBus bus_;
    vis::Renderer     renderer_;
};

} // namespace app
