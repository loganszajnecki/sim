// Integrator.hpp
#pragma once

#include "sim/State.hpp"
#include <functional>

namespace sim {

/**
 * @brief Type alias for a state derivative function.
 *
 * The function computes dy/dt at time t for state y.
 *
 * Requirements:
 *  - The returned State should have the same size as the input y.
 *  - The time field `t` in the returned State is typically set to the input `t`,
 *    but the integrator does not depend on it (it advances time itself).
 */
using DerivFunc = std::function<State(double /*t*/, const State& /*y*/)>;

/**
 * @brief Classic 4th-order Runge–Kutta integrator (RK4).
 *
 * This class provides a single static method for taking one RK4 step:
 *   y_{n+1} = y_n + h * f(t_n, y_n) + ...
 *
 * It assumes:
 *  - State::x is a flat vector of doubles.
 *  - DerivFunc returns a State with the same size as the input.
 */
class RK4Integrator 
{
public:
    /**
     * @brief Take a single RK4 integration step.
     *
     * @param f Derivative function f(t, y) = dy/dt.
     * @param t Current time t_n.
     * @param y Current state y_n.
     * @param h Time step size.
     *
     * @return State y_{n+1} at time t + h.
     */
    [[nodiscard]] static State step(const DerivFunc& f,
                                    double t,
                                    const State& y,
                                    double h);
};

} // namespace sim
