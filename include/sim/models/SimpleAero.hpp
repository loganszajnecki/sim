#pragma once

#include "sim/Components.hpp"

namespace models {

/**
 * @brief Simple placeholder aerodynamics and propulsion model (3-DoF).
 *
 * This model provides a highly simplified representation of missile forces.
 * It interprets the incoming actuator command (from the autopilot) as a
 * desired specific acceleration vector [ax, ay, az], and converts it into
 * total body-frame forces using three basic effects:
 *
 *   1. **Constant thrust** — adds a fixed acceleration along +X (forward) to
 *      represent engine thrust.
 *   2. **Scaled control response** — multiplies the commanded acceleration
 *      components by a gain to control how strongly the missile responds to
 *      autopilot inputs.
 *   3. **Linear velocity damping** — subtracts a term proportional to velocity
 *      in each axis to mimic aerodynamic drag or control damping.
 *
 * This model outputs translational forces only (no rotational moments),
 * making it suitable for a 3-DoF point-mass simulation.
 *
 * ### Parameters
 * @param accelGain
 *   Scalar multiplier applied to the actuator command.
 *   Higher values make the missile more responsive to guidance commands.
 *
 * @param velDamping
 *   Linear damping coefficient applied to velocity components (mimics drag).
 *   Higher values cause more resistance and slower acceleration at high speed.
 *
 * @param thrustAccel
 *   Constant forward acceleration (in +X direction) representing thrust.
 *   This is a simple stand-in for engine force divided by missile mass.
 */
class SimpleAero : public sim::IAerodynamics
{
public:
    SimpleAero(double accelGain   = 1.0,
               double velDamping  = 0.0,
               double thrustAccel = 0.0,
               double gravityAccel = 9.81);
    
    /**
     * @brief Compute aerodynamic and propulsive forces.
     *
     * @param state        Current missile state (position, velocity, etc.).
     * @param actuatorCmd  Command vector [ax, ay, az] from the autopilot.
     * @return             Aerodynamic/propulsive forces (and zeroed moments).
     */
    sim::AeroForces compute(const sim::State& state, 
                            const std::vector<double>& actuatorCmd) override;
private:
    double accelGain_;    // scales commanded acceleration
    double velDamping_;   // linear damping coefficient on velocity
    double thrustAccel_;  // constant forward (x-axis) thrust acceleration
    double gravityAccel_;  // +Z is world up
};

} // namespace models