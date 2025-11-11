// Missile.cpp
#include "sim/Missile.hpp"
#include <cassert>

namespace sim {

Missile::Missile()
    : state(Indices::Size) // pos(3), vel(3)
{
}

State Missile::derivative(double /*t*/,
                          const State& s,
                          const std::vector<double>& actuatorCommands)
{
    assert(s.size() == Indices::Size);

    State ds(Indices::Size);
    ds.t = s.t; // time is advanced by the integrator, not here

    // Position derivatives are the current velocities.
    ds.x[Indices::PX] = s.x[Indices::VX];
    ds.x[Indices::PY] = s.x[Indices::VY];
    ds.x[Indices::PZ] = s.x[Indices::VZ];

    // Simple mass and force model: acc = F / m, with m = 1 for now.
    AeroForces af{ {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };

    if (aero) {
        af = aero->compute(s, actuatorCommands);
    }

    // Velocity derivatives are the accelerations.
    ds.x[Indices::VX] = af.force[0];
    ds.x[Indices::VY] = af.force[1];
    ds.x[Indices::VZ] = af.force[2];

    return ds;
}

} // namespace sim
