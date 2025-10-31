#include "sim/Missile.hpp"
#include <cassert>

namespace sim {

Missile::Missile() {
    state = State(6); // pos(3), vel(3)
}

State Missile::derivative(double /*t*/, const State& s, const std::vector<double>& actuatorCommands)
{
    assert(s.size() == 6);
    State ds(6);
    ds.t = s.t;

    // positions derivatives (vel)
    ds.x[0] = s.x[3];
    ds.x[1] = s.x[4];
    ds.x[2] = s.x[5];

    // simple mass and force model: acc = F/m, m=1 for now
    AeroForces af = { {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };
    if (aero) af = aero->compute(s, actuatorCommands);

    ds.x[3] = af.force[0];
    ds.x[4] = af.force[1];
    ds.x[5] = af.force[2];
    
    return ds;
}

} // namespace sim