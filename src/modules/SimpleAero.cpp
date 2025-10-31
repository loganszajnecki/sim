#include "modules/SimpleAero.hpp"

namespace modules {

SimpleAero::SimpleAero(double accGain, double damping, double thrustAcc) 
    : gain_(accGain), damping_(damping), thrustAcc_(thrustAcc) {}

// actuators are interpreted as desired acc vector [ax, ay, az]
sim::AeroForces SimpleAero::compute(const sim::State& s, const std::vector<double>& actuators)
{
    sim::AeroForces out;
    if (actuators.size() >= 3) {
        out.force[0] = thrustAcc_ + gain_ * actuators[0] - damping_ * s.x[3];
        out.force[1] =            + gain_ * actuators[1] - damping_ * s.x[4];
        out.force[2] =            + gain_ * actuators[2] - damping_ * s.x[5];
    }
    return out;
}

} // namespace modules