// ProNav.hpp
#pragma once

#include "sim/Components.hpp"
#include "sim/State.hpp"
#include <algorithm>
#include <vector>
#include <cstddef>

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
 * Assumes a 3-DoF state layout:
 *   x[0..2] : position (px, py, pz)
 *   x[3..5] : velocity (vx, vy, vz)
 */
class ProNav : public sim::IGuidance
{
public:
    explicit ProNav(double N = 3.0, double amax = 100.0)
        : N_(N), amax_(amax) {}

    /**
     * @brief Compute a ProNav guidance acceleration command.
     *
     * @param m  Current missile state.
     * @param t  Current target state.
     * @return   Desired specific acceleration vector [ax, ay, az] in the
     *           simulation frame, saturated to have norm <= amax_.
     *           Returns a zero vector if range is too small.
     */
    [[nodiscard]] std::vector<double> computeGuidanceCommand(
        const sim::State& m,
        const sim::State& t) override;

private:
    // Indices into the 3-DoF state vector.
    static constexpr std::size_t PX_IDX = 0;
    static constexpr std::size_t PY_IDX = 1;
    static constexpr std::size_t PZ_IDX = 2;
    static constexpr std::size_t VX_IDX = 3;
    static constexpr std::size_t VY_IDX = 4;
    static constexpr std::size_t VZ_IDX = 5;

    double N_;     ///< Navigation constant (ProNav gain).
    double amax_;  ///< Maximum allowed acceleration magnitude (saturation limit).
};

} // namespace models
