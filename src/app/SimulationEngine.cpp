// SimulationEngine.cpp
#include "app/SimulationEngine.hpp"

#include "sim/Integrator.hpp"

#include <cmath>
#include <cassert>

namespace app {

SimulationEngine::SimulationEngine(sim::Missile m,
                                   sim::State target0,
                                   double intercept_radius_m)
    : missile_(std::move(m))
    , s_{}              // will be set via setInitial
    , tgt_(std::move(target0))
    , t_{0.0}
    , r2_thresh_{intercept_radius_m * intercept_radius_m}
{
}

void SimulationEngine::setInitial(const sim::State& s0, double t0)
{
    s_   = s0;
    s_.t = t0;
    t_   = t0;

    // Keep target time consistent as well.
    tgt_.t = t0;

    latched_     = false;
    latched_val_ = {};
}

std::optional<InterceptResult> SimulationEngine::step(double h)
{
    // -------------------------------------------------------------------------
    // 1) Constant-velocity target propagation.
    // -------------------------------------------------------------------------
    tgt_.x[0] += tgt_.x[3] * h;
    tgt_.x[1] += tgt_.x[4] * h;
    tgt_.x[2] += tgt_.x[5] * h;
    tgt_.t     = t_ + h; // keep target time in sync (approx.)

    // -------------------------------------------------------------------------
    // 2) Guidance + autopilot.
    // -------------------------------------------------------------------------
    assert(missile_.guidance && "Guidance model must be configured before step().");
    assert(missile_.autopilot && "Autopilot model must be configured before step().");

    // Guidance command: desired specific acceleration from guidance law.
    const auto desiredAccel = missile_.guidance->computeGuidanceCommand(s_, tgt_);

    // Actuator command: shaped by the autopilot.
    const auto actuatorCmd = missile_.autopilot->computeActuatorCommand(s_, desiredAccel);

    // -------------------------------------------------------------------------
    // 3) Missile dynamics (RK4 at fixed h).
    // -------------------------------------------------------------------------
    sim::DerivFunc f = [&](double /*tt*/, const sim::State& yy) {
        // Current missile derivative does not depend explicitly on time,
        // so we ignore the integrator's time argument and use t_ instead.
        return missile_.derivative(t_, yy, actuatorCmd);
    };

    sim::State snew = sim::RK4Integrator::step(f, t_, s_, h);
    snew.t = t_ + h;

    s_ = std::move(snew);
    t_ += h;

    // -------------------------------------------------------------------------
    // 4) Intercept test (distance between missile and target).
    // -------------------------------------------------------------------------
    const double dx = s_.x[0] - tgt_.x[0];
    const double dy = s_.x[1] - tgt_.x[1];
    const double dz = s_.x[2] - tgt_.x[2];
    const double d2 = dx * dx + dy * dy + dz * dz;

    if (!latched_ && d2 <= r2_thresh_) {
        latched_ = true;
        latched_val_ = { t_, std::sqrt(d2) };
        return latched_val_;
    }

    if (latched_) {
        return latched_val_;
    }

    return std::nullopt;
}

} // namespace app
