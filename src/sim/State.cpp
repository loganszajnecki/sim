// State.cpp
#include "sim/State.hpp"

namespace sim {

State operator+(const State& a, const State& b)
{
    assert(a.size() == b.size());
    State r(a.size());
    r.t = a.t; // time is managed separately by the integrator
    for (std::size_t i = 0; i < a.size(); ++i) {
        r.x[i] = a.x[i] + b.x[i];
    }
    return r;
}

State operator-(const State& a, const State& b)
{
    assert(a.size() == b.size());
    State r(a.size());
    r.t = a.t; // time is managed separately by the integrator
    for (std::size_t i = 0; i < a.size(); ++i) {
        r.x[i] = a.x[i] - b.x[i];
    }
    return r;
}

State scaled(const State& a, double s)
{
    State r(a.size());
    r.t = a.t; // keep same time; scaling is purely spatial/state
    for (std::size_t i = 0; i < a.size(); ++i) {
        r.x[i] = a.x[i] * s;
    }
    return r;
}

} // namespace sim
