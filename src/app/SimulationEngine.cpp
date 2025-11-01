#include <cmath>
#include "app/SimulationEngine.hpp"
#include "sim/Integrator.hpp"

using namespace sim;

namespace app {

SimulationEngine::SimulationEngine(Missile m, State target0, double r)
    : missile_(std::move(m)), tgt_(target0), r2_thresh_(r*r) {}

void SimulationEngine::setInitial(const State& s0, double t0) {
    s_ = s0;
    s_.t = t0;
    t_ = t0;
}

std::optional<InterceptResult> SimulationEngine::step(double h) {
    // 1) constant-velocity target
    tgt_.x[0] += tgt_.x[3]*h; 
    tgt_.x[1] += tgt_.x[4]*h; 
    tgt_.x[2] += tgt_.x[5]*h;

    // 2) guidance + autopilot
    auto gcmd = missile_.guidance->guidanceCommand(s_, tgt_);
    auto ctrl = missile_.autopilot->control(s_, gcmd);
   
    // 3) dynamics (RK4 at fixed h)
    DerivFunc f = [&](double /*tt*/, const State& yy) { return missile_.derivative(t_, yy, ctrl); };
    State snew = RK4Integrator::step(f, t_, s_, h);
    snew.t = t_ + h;

    s_ = snew;
    t_ += h;

    // 4) intercept test
    double dx = s_.x[0]-tgt_.x[0];
    double dy = s_.x[1]-tgt_.x[1];
    double dz = s_.x[2]-tgt_.x[2];
    double d2 = dx*dx + dy*dy + dz*dz;
    
    if (!latched_ && d2 <= r2_thresh_) { 
        latched_ = true; 
        latched_val_ = {t_, std::sqrt(d2)}; 
        return latched_val_;
    }

    if (latched_) {
        return latched_val_;
    }
    return std::nullopt;
}

} // namespace app