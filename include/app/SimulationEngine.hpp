// SimulationEngine.hpp
#pragma once

#include "sim/State.hpp"
#include "sim/Missile.hpp"

#include <optional>

namespace app {

/**
 * @brief Result of an intercept event.
 *
 * Once an intercept is detected (missile-target distance <= intercept radius),
 * the result is "latched": subsequent calls to step() continue to return
 * the same InterceptResult until the simulation ends.
 */
struct InterceptResult {
    double t{};     ///< Intercept time [s] (approximate).
    double poca{};  ///< Point-of-closest-approach distance [m].
};

/**
 * @brief Thin wrapper around the core simulation objects.
 *
 * Responsibilities:
 *  - Owns a configured sim::Missile (with its models).
 *  - Owns the missile and target states.
 *  - Advances both missile and target forward in time using a fixed step h.
 *  - Detects intercept when the missile comes within a fixed radius of the target.
 *
 * Assumptions:
 *  - Target follows a simple constant-velocity model.
 *  - Missile.holds guidance and autopilot pointers that are valid (non-null)
 *    before step() is called.
 *  - State layout is the current 3-DoF convention:
 *      x[0..2] : position (px, py, pz)
 *      x[3..5] : velocity (vx, vy, vz)
 */
class SimulationEngine
{
public:
    /**
     * @brief Construct a simulation engine.
     *
     * @param m                  Configured missile (ownership is moved in).
     * @param target0            Initial target state.
     * @param intercept_radius_m Intercept radius [m]; if distance between
     *                           missile and target is <= this, an intercept
     *                           is declared.
     */
    explicit SimulationEngine(sim::Missile m,
                              sim::State target0,
                              double intercept_radius_m = 5.0);

    /**
     * @brief Set the initial missile state and time.
     *
     * @param s0 Initial missile state.
     * @param t0 Initial simulation time [s].
     */
    void setInitial(const sim::State& s0, double t0);

    /**
     * @brief Advance the simulation by a fixed time step.
     *
     * Target is advanced with constant velocity; missile uses RK4 integration
     * with its current guidance and autopilot models.
     *
     * @param h Time step [s].
     * @return  InterceptResult if an intercept has occurred (latched), or
     *          std::nullopt if no intercept has been detected yet.
     */
    [[nodiscard]] std::optional<InterceptResult> step(double h);

    /// Current missile state (missile position/velocity).
    [[nodiscard]] const sim::State& missile() const noexcept { return s_; }

    /// Current target state.
    [[nodiscard]] const sim::State& target() const noexcept { return tgt_; }

private:
    sim::Missile missile_;  ///< Missile model and associated components.
    sim::State   s_{};      ///< Missile state.
    sim::State   tgt_{};    ///< Target state.

    double t_{0.0};         ///< Current simulation time [s].
    double r2_thresh_{0.0}; ///< Squared intercept radius [m^2].

    bool            latched_{false};
    InterceptResult latched_val_{};
};

} // namespace app
