#pragma once
#include <chrono>

namespace app {

// Drives a fixed-dt simulation using a real-time accumulator.
// Usage per frame: auto plan = loop.beginFrame(); then run plan.steps times with step size plan.h.
// Call resetLag() if you want to stop catch-up after a latched event (e.g., intercept).
class LoopController {
public:
    struct FramePlan { int steps; double h; };

    explicit LoopController(double h, int maxStepsPerFrame = 10)
        : h_(h), maxSteps_(maxStepsPerFrame), lastWall_(Clock::now()) {}

    FramePlan beginFrame() {
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

    void resetLag() { lag_ = 0.0; }

    void setStep(double h) { h_ = h; }
    void setMaxStepsPerFrame(int n) { maxSteps_ = n; }

    // Pause/speed control (0 = paused, 1 = realtime, 2 = 2x)
    void setTimeScale(double s) { timeScale_ = s; }
    double timeScale() const { return timeScale_; }

private:
    using Clock = std::chrono::steady_clock;

    double h_;
    int    maxSteps_;
    Clock::time_point lastWall_;
    double lag_ = 0.0;
    double timeScale_ = 1.0;
};

} // namespace app