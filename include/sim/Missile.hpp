// Missile.hpp
#pragma once

#include "sim/State.hpp"
#include "sim/Components.hpp"
#include <memory>
#include <vector>

namespace sim {

/**
 * @brief Simple 3-DoF missile model wrapper.
 *
 * This class owns the aerodynamic, guidance, and autopilot models used to
 * propagate the missile state forward in time.
 *
 * Current state layout (size = 6):
 *   x[0..2] : position in world coordinates  (px, py, pz)
 *   x[3..5] : velocity in world coordinates  (vx, vy, vz)
 *
 * Notes:
 *  - Mass is implicitly 1.0 in the current force model (acc = F / m, m = 1).
 *  - Gravity and higher-fidelity dynamics can be added later in the derivative().
 */
class Missile
{
public:
    /// Indices into the current 3-DoF state vector.
    struct Indices {
        static constexpr std::size_t PX = 0;
        static constexpr std::size_t PY = 1;
        static constexpr std::size_t PZ = 2;
        static constexpr std::size_t VX = 3;
        static constexpr std::size_t VY = 4;
        static constexpr std::size_t VZ = 5;
        static constexpr std::size_t Size = 6;
    };

    Missile();

    ~Missile() = default;

    Missile(const Missile&) = delete;
    Missile& operator=(const Missile&) = delete;

    Missile(Missile&&) noexcept = default;
    Missile& operator=(Missile&&) noexcept = default;

    /// Public state of the missile (position/velocity in world frame).
    State state;

    /// Owned models. These are typically injected/configured externally.
    std::unique_ptr<IAerodynamics> aero;
    std::unique_ptr<IGuidance>    guidance;
    std::unique_ptr<IAutopilot>   autopilot;

    /**
     * @brief Compute time-derivative of the state.
     *
     * @param t   Current simulation time [s]. (Currently unused but kept for future use.)
     * @param s   Current state (size must be Indices::Size).
     * @param actuatorCommands Autopilot/guidance outputs (interpretation depends on model).
     *
     * @return State ds/dt evaluated at (t, s).
     */
    [[nodiscard]] State derivative(double t,
                                   const State& s,
                                   const std::vector<double>& actuatorCommands);
};

} // namespace sim
