#pragma once

#include "sim/Components.hpp"

namespace modules {

/**
 * @brief Simple placeholder autopilot for 3-DoF simulations.
 *
 * This module acts as a lightweight "bridge" between the guidance law
 * (e.g., ProNav) and the missile’s aerodynamic model. It receives a
 * guidance-generated *desired acceleration vector* and produces a shaped
 * *actuator command* vector that the aerodynamic model can use directly.
 *
 * In this simplified implementation, the autopilot does not perform
 * feedback control or dynamic response modeling. Instead, it applies:
 *
 *   1. **Gain scaling** — multiplies each component of the guidance
 *      acceleration by a fixed scalar gain, adjusting control authority.
 *   2. **Symmetric saturation** — clamps each axis to ±maxCmd_ to prevent
 *      excessive actuator commands or unrealistic accelerations.
 *
 * This placeholder represents a single-loop approximation and is intended
 * to be replaced by a full three-loop autopilot (acceleration → attitude →
 * rate → actuator) in future 6-DoF models.
 *
 * ### Parameters
 * @param gain
 *   Scalar multiplier applied to the incoming guidance acceleration command.
 *   Controls how aggressively the missile responds to guidance inputs.
 *
 * @param maxCmd
 *   Symmetric saturation limit applied per-axis (±maxCmd). Prevents the
 *   autopilot from commanding unrealistically large accelerations.
 */
class SimpleAccelAutopilot : public sim::IAutopilot
{
public:
    SimpleAccelAutopilot(double gain = 1.0, double maxCmd = 100.0);

    /**
     * @brief Compute actuator commands from a desired acceleration vector.
     *
     * @param state        Current missile state (unused in this placeholder).
     * @param desiredAccel Guidance-generated acceleration command [ax, ay, az].
     * @return             Shaped actuator command vector (gain + saturation applied).
     */
    std::vector<double> computeActuatorCommand(
        const sim::State& state,
        const std::vector<double>& desiredAccel) override;
private:
    double gain_;    // scalar gain on guidance accel
    double maxCmd_;  // symmetric saturation per axis (±maxCmd_)

};

} // namespace modules