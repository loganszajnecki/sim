#include "modules/SimpleAero.hpp"

namespace modules {

SimpleAero::SimpleAero(double accelGain, double velDamping, double thrustAccel, double gravityAccel)
    : accelGain_(accelGain), velDamping_(velDamping), thrustAccel_(thrustAccel),
      gravityAccel_(gravityAccel) {}

// actuators are interpreted as desired acc vector [ax, ay, az]
sim::AeroForces SimpleAero::compute(const sim::State& state, 
                                    const std::vector<double>& actuatorCmd)
{
    sim::AeroForces out{}; // initializes both force and moment to zero

    if (actuatorCmd.size() >= 3) {
        const double vx = state.x[3];
        const double vy = state.x[4];
        const double vz = state.x[5];

        // Translational forces (mass-normalized)
        out.force[0] = thrustAccel_ + accelGain_ * actuatorCmd[0] - velDamping_ * vx;
        out.force[1] =                accelGain_ * actuatorCmd[1] - velDamping_ * vy;
        out.force[2] =                accelGain_ * actuatorCmd[2] - velDamping_ * vz - gravityAccel_;
        
        // Rotational moments: not modeled in 3-DoF
        out.moment = {0.0, 0.0, 0.0};
    }
    
    return out;
}

} // namespace modules