// SimpleAero.cpp
#include "sim/models/SimpleAero.hpp"
#include <cassert>

namespace models {

SimpleAero::SimpleAero(double accelGain,
                       double velDamping,
                       double thrustAccel,
                       double gravityAccel)
    : accelGain_(accelGain)
    , velDamping_(velDamping)
    , thrustAccel_(thrustAccel)
    , gravityAccel_(gravityAccel)
{
}

sim::AeroForces SimpleAero::compute(const sim::State& state,
                                    const std::vector<double>& actuatorCmd)
{
    sim::AeroForces out{}; // initializes both force and moment to zero

    // Require a minimal 3-DoF state layout: pos(3), vel(3).
    assert(state.size() >= VZ_IDX + 1);

    if (actuatorCmd.size() >= 3) {
        const double vx = state.x[VX_IDX];
        const double vy = state.x[VY_IDX];
        const double vz = state.x[VZ_IDX];

        // Translational forces (mass-normalized, i.e., accelerations).
        out.force[0] = thrustAccel_
                     + accelGain_ * actuatorCmd[0]
                     - velDamping_ * vx;

        out.force[1] = accelGain_ * actuatorCmd[1]
                     - velDamping_ * vy;

        // +Z is world up; gravity acts downward in -Z, so subtract gravityAccel_.
        out.force[2] = accelGain_ * actuatorCmd[2]
                     - velDamping_ * vz
                     - gravityAccel_;

        // Rotational moments: not modeled in 3-DoF.
        out.moment = {0.0, 0.0, 0.0};
    }

    return out;
}

} // namespace models
