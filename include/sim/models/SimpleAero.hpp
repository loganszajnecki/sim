// SimpleAero.hpp
#pragma once

#include "sim/Components.hpp"
#include "sim/State.hpp"
#include <vector>

namespace models {

/**
 * @brief Simple placeholder aerodynamics and propulsion model (3-DoF).
 *
 * This model provides a highly simplified representation of missile forces.
 * It interprets the incoming actuator command (from the autopilot) as a
 * desired specific acceleration vector [ax, ay, az], and converts it into
 * total forces using three basic effects:
 *
 *   1. Constant thrust — adds a fixed acceleration along +X (forward) to
 *      represent engine thrust.
 *   2. Scaled control response — multiplies the commanded acceleration
 *      components by a gain to control how strongly the missile responds
 *      to autopilot inputs.
 *   3. Linear velocity damping — subtracts a term proportional to velocity
 *      in each axis to mimic aerodynamic drag or control damping.
 *
 * This model outputs translational forces only (no rotational moments),
 * making it suitable for a 3-DoF point-mass simulation.
 *
 * Coordinate convention:
 *   - World frame, +Z is up.
 *   - Gravity is applied as a constant downward acceleration in -Z.
 */
class SimpleAero : public sim::IAerodynamics
{
public:
    SimpleAero(double accelGain    = 1.0,
               double velDamping   = 0.0,
               double thrustAccel  = 0.0,
               double gravityAccel = 9.81);

    /**
     * @brief Compute aerodynamic and propulsive forces.
     *
     * @param state        Current missile state (expects at least pos(3), vel(3)).
     * @param actuatorCmd  Command vector [ax, ay, az] from the autopilot.
     * @return             Aerodynamic/propulsive forces (and zeroed moments).
     */
    [[nodiscard]] sim::AeroForces compute(const sim::State& state,
                                          const std::vector<double>& actuatorCmd) override;

private:
    // State indices (must match the 3-DoF layout used elsewhere).
    static constexpr std::size_t VX_IDX = 3;
    static constexpr std::size_t VY_IDX = 4;
    static constexpr std::size_t VZ_IDX = 5;

    double accelGain_;     // scales commanded acceleration
    double velDamping_;    // linear damping coefficient on velocity
    double thrustAccel_;   // constant forward (x-axis) thrust acceleration
    double gravityAccel_;  // magnitude of gravitational acceleration (+Z is up)
};

} // namespace models
