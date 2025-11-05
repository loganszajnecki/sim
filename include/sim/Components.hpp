#pragma once

#include "sim/State.hpp"
#include <vector>
#include <array>
#include <memory>

namespace sim {

struct AeroForces {
    std::array<double, 3> force{0.0, 0.0, 0.0};
    std::array<double, 3> moment{0.0, 0.0, 0.0};
};

class IAerodynamics
{
public:
    virtual ~IAerodynamics() = default;

    /**
     * @brief Compute aerodynamic and propulsive forces and moments.
     *
     * This interface is intentionally simple for the current 3-DoF point-mass model.
     *
     * @param state         Current missile state (position, velocity, etc.).
     * @param actuatorCmd   Command vector from the autopilot. In the current 3-DoF
     *                      model, this represents a desired specific acceleration
     *                      vector [ax, ay, az] in the simulation frame.
     * @return              Structure containing total aerodynamic/propulsive
     *                      forces (and optionally moments).
     *
     * In future 6-DoF models, implementations can compute both forces and moments
     * using attitude, angular rates, Mach number, angle-of-attack, etc.
     */
    virtual AeroForces compute(
        const State& state, 
        const std::vector<double>& actuatorCmd) = 0;
};

class IGuidance
{
public:
    virtual ~IGuidance() = default;

    /**
     * @brief Compute a guidance command for the missile.
     *
     * @param missileState  Current missile state.
     * @param targetState   Current target state (or estimate).
     * @return              Guidance command vector.
     *                      In the current model, this is a desired specific
     *                      acceleration [ax, ay, az] in the simulation frame.
     */
    virtual std::vector<double> computeGuidanceCommand(const State& s, const State& target) = 0;
};

class IAutopilot
{
public:
    virtual ~IAutopilot() = default;
    
    /**
     * @brief Convert a guidance acceleration command into actuator commands.
     *
     * This is intentionally a SIMPLE placeholder interface for the current 3-DoF model.
     * - desiredAccel: 3-element vector [ax, ay, az] from the guidance law
     *                 (interpretation: desired specific acceleration).
     * - Return value: actuator command vector consumed by the missile dynamics/aero model.
     *
     * In the current 3-DoF implementation, this may just be a scaled / saturated
     * version of desiredAccel. In a future 6-DoF, a full 3-loop autopilot can
     * implement this same interface with real attitude/rate/actuator dynamics.
     */
    virtual std::vector<double> computeActuatorCommand(
        const State& state, 
        const std::vector<double>& desiredAccel) = 0;
};

} // namespace sim