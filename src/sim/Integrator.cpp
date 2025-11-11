// Integrator.cpp
#include "sim/Integrator.hpp"
#include <cassert>

namespace sim {

State RK4Integrator::step(const DerivFunc& f,
                          double t,
                          const State& y,
                          double h)
{
    // k1 = f(t, y)
    State k1 = f(t, y);
    assert(k1.size() == y.size());

    // k2 = f(t + h/2, y + h/2 * k1)
    State y2 = y + scaled(k1, 0.5 * h);
    State k2 = f(t + 0.5 * h, y2);
    assert(k2.size() == y.size());

    // k3 = f(t + h/2, y + h/2 * k2)
    State y3 = y + scaled(k2, 0.5 * h);
    State k3 = f(t + 0.5 * h, y3);
    assert(k3.size() == y.size());

    // k4 = f(t + h, y + h * k3)
    State y4 = y + scaled(k3, h);
    State k4 = f(t + h, y4);
    assert(k4.size() == y.size());

    const std::size_t n = y.size();
    State out(n);
    out.t = y.t + h; // Integrator is responsible for advancing time.

    // y_{n+1} = y_n + h/6 * (k1 + 2*k2 + 2*k3 + k4)
    for (std::size_t i = 0; i < n; ++i) {
        out.x[i] = y.x[i]
                 + (h / 6.0) * (k1.x[i]
                              + 2.0 * k2.x[i]
                              + 2.0 * k3.x[i]
                              + k4.x[i]);
    }

    return out;
}

} // namespace sim
