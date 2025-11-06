#pragma once
#include "sim/Components.hpp"
#include <algorithm>

namespace models {

/**
 * @brief 3D Proportional Navigation (PN) guidance law.
 *
 * Implements a classical 3D proportional navigation (ProNav) algorithm to
 * generate a lateral acceleration command that drives the missile toward
 * the target. The commanded acceleration is orthogonal to the line-of-sight
 * (LOS) and proportional to LOS rate.
 *
 * The implemented form is:
 *
 *   a_cmd = (N * Vc / |r|) * [ los_hat × (v_rel × los_hat) ]
 *
 * where
 *   - r       = target position - missile position
 *   - v_rel   = target velocity - missile velocity
 *   - |r|     = range
 *   - los_hat = unit line-of-sight vector r / |r|
 *   - Vc      = closing speed = -dot(v_rel, los_hat)
 *
 * The resulting commanded acceleration vector is then saturated to a
 * maximum magnitude amax_.
 *
 * ### Parameters
 * @param N
 *   Navigation constant (dimensionless). Typical values are in the range
 *   3–5. Larger N increases responsiveness and curvature of the missile
 *   trajectory toward the target.
 *
 * @param amax
 *   Maximum allowed magnitude of the commanded acceleration. Used to
 *   saturate the output to represent missile g-limits or actuator limits.
 */
class ProNav : public sim::IGuidance
{
public:
    explicit ProNav(double N = 3.0, double amax = 100.0) : N_(N), amax_(amax) {}

    /**
     * @brief Compute a ProNav guidance acceleration command.
     *
     * @param m  Current missile state.
     * @param t  Current target state.
     * @return   Desired specific acceleration vector [ax, ay, az] in the
     *           simulation frame, saturated to have norm <= amax_.
     */
    std::vector<double>computeGuidanceCommand(const sim::State& m, const sim::State& t) override;
private:
    double N_;     // Navigation constant (ProNav gain).
    double amax_;  // Maximum allowed acceleration magnitude (saturation limit).
};

} // namespace models