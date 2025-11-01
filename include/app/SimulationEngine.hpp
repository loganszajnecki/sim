#pragma once
#include "sim/State.hpp"
#include "sim/Missile.hpp"
#include <optional>

namespace app {

struct InterceptResult {
    double t{};     // intercept time (s)
    double poca{};  // POCA distance (m)
};

class SimulationEngine
{
public:
    SimulationEngine(sim::Missile m, sim::State target0, double intercept_radius_m = 5.0);
    void setInitial(const sim::State& s0, double t0);

    // advance by fixed h; returns intercept when latched
    std::optional<InterceptResult> step(double h);
    
    const sim::State& missile() const { return s_; }
    const sim::State& target() const { return tgt_; }

private:
    sim::Missile missile_;
    sim::State s_{};
    sim::State tgt_{};
    double t_{0.0};
    double r2_thresh_{0.0};
    bool latched_{false};
    InterceptResult latched_val_{};
};

} // namespace app