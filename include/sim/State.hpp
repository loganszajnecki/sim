// State.hpp
#pragma once

#include <vector>
#include <cstddef>
#include <cassert>

namespace sim {

/**
 * @brief Generic simulation state container.
 *
 * Holds the simulation time `t` and a generalized state vector `x`.
 *
 * The layout of `x` is defined by the model using it.
 * For example, in the current missile models:
 *   - 3-DoF: [0..2] position (x, y, z), [3..5] velocity (vx, vy, vz)
 *   - 6-DoF: will extend this with attitude, angular rates, etc.
 */
struct State {
    double t{0.0};               ///< Simulation time [s]
    std::vector<double> x;       ///< Generalized state vector

    /// Default-constructed state has t = 0 and an empty state vector.
    State() = default;

    /// Construct a state with `n` elements, initialized to `initialValue` (default 0.0).
    explicit State(std::size_t n, double initialValue = 0.0)
        : t(0.0), x(n, initialValue) {}

    /// Number of state elements.
    [[nodiscard]] std::size_t size() const noexcept { return x.size(); }

    /**
     * @brief Resize and (re)initialize the state vector.
     *
     * This is a convenience for callers that want to reuse a State instance
     * without worrying about prior contents.
     */
    void resize(std::size_t n, double value = 0.0) {
        x.assign(n, value);
    }
};

// Arithmetic helpers used by the integrator / RK4 operations.
// Note: the time component `t` is taken from the left-hand operand.
// Integrators are responsible for advancing `t` explicitly.

[[nodiscard]] State operator+(const State& a, const State& b);
[[nodiscard]] State operator-(const State& a, const State& b);
[[nodiscard]] State scaled(const State& a, double s);

} // namespace sim
