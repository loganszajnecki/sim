#pragma once

#include "sim/State.hpp"
#include <functional>

namespace sim {

using DerivFunc = std::function<State(double, const State&)>;

class RK4Integrator 
{
public:
    // single step rk4
    static State step(const DerivFunc& f, double t, const State& y, double h);
};

} // namespace sim