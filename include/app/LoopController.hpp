#pragma once

#include <chrono>

namespace app {

/**
 * @brief Drives a fixed-dt simulation using a real-time accumulator.
 *
 * Typical usage per frame:
 *   auto plan = loop.beginFrame();
 *   for (int i = 0; i < plan.steps; ++i) {
 *       // advance simulation by plan.h
 *   }
 *
 * Call resetLag() if you want to stop catch-up behavior after a latched event
 * (e.g., after an intercept is detected and the sim should no longer run faster
 * than real time to catch up).
 */
class LoopController {
public:
    struct FramePlan {
        int    steps{0};  ///< Number of fixed steps to run this frame.
        double h{0.0};    ///< Fixed time step [s].
    };

    /**
     * @param h                 Fixed simulation time step [s].
     * @param maxStepsPerFrame  Safety cap on how many steps to run per frame
     *                          when catching up.
     */
    explicit LoopController(double h, int maxStepsPerFrame = 10)
        : h_(h)
        , maxSteps_(maxStepsPerFrame)
        , lastWall_(Clock::now())
    {
    }

    /**
     * @brief Compute how many steps to run this frame based on wall-clock time.
     *
     * Accumulates real time since the previous frame (scaled by timeScale),
     * converts that into an integer number of fixed steps, and leaves any
     * fractional remainder in the internal lag accumulator.
     */
    [[nodiscard]] FramePlan beginFrame()
    {
        const auto now = Clock::now();
        const double dt = std::chrono::duration<double>(now - lastWall_).count();
        lastWall_ = now;

        lag_ += dt * timeScale_;

        int steps = 0;
        while (lag_ >= h_ && steps < maxSteps_) {
            lag_ -= h_;
            ++steps;
        }

        return {steps, h_};
    }

    /// Reset any accumulated lag so the controller stops trying to catch up.
    void resetLag() { lag_ = 0.0; }

    /// Reset lag and wall-clock baseline.
    void resetTimeBase()
    {
        lastWall_ = Clock::now();
        lag_      = 0.0;
    }

    /// Set the fixed time step [s].
    void setStep(double h) { h_ = h; }

    /// Set the maximum number of steps per frame.
    void setMaxStepsPerFrame(int n) { maxSteps_ = n; }

    /// Pause/speed control (0 = paused, 1 = realtime, 2 = 2x, etc.).
    void setTimeScale(double s) { timeScale_ = s; }

    [[nodiscard]] double timeScale() const noexcept { return timeScale_; }

private:
    using Clock = std::chrono::steady_clock;

    double h_{0.0};
    int    maxSteps_{10};
    Clock::time_point lastWall_{Clock::now()};
    double lag_{0.0};
    double timeScale_{1.0};
};

} // namespace app
