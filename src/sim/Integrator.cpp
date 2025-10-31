#include "sim/Integrator.hpp"
#include "sim/State.hpp"

namespace sim {

State RK4Integrator::step(const DerivFunc &f, double t, const State &y, double h)
{
    State k1 = f(t, y);
    State y2 = y + scaled(k1, 0.5*h);
    State k2 = f(t + 0.5*h, y2);
    State y3 = y + scaled(k2, 0.5*h);
    State k3 = f(t + 0.5*h, y3);
    State y4 = y + scaled(k3, h);
    State k4 = f(t + h, y4);

    State out(y.size());
    out.t = y.t + h;
    for (size_t i = 0; i < y.size(); ++i) {
        out.x[i] = y.x[i] + (h/6.0) * (k1.x[i] + 2.0*k2.x[i] + 2.0*k3.x[i] + k4.x[i]);
    }
    return out;
}

} // namespace sim