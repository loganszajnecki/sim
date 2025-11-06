#pragma once

#include "sim/State.hpp"
#include "sim/Components.hpp"
#include <memory>

namespace sim {

class Missile
{
public:
    Missile();

    // public state and models
    State state;

    std::unique_ptr<IAerodynamics> aero;
    std::unique_ptr<IGuidance> guidance;
    std::unique_ptr<IAutopilot> autopilot;

    // compute derivative given current state and actuator commands
    State derivative(double t, const State& s, const std::vector<double>& actuatorCommands);
};

} // namespace sim