#pragma once

#include "sim/State.hpp"
#include <vector>
#include <array>
#include <memory>

namespace sim {

struct AeroForces {
    std::array<double, 3> force{0.0, 0.0, 0.0};
    std::array<double, 3> moment{0.0, 0.0, 0.0};
};

class IAerodynamics
{
public:
    virtual ~IAerodynamics() = default;
    virtual AeroForces compute(const State& s, const std::vector<double>& actuators) = 0;
};

class IGuidance
{
public:
    virtual ~IGuidance() = default;
    // compute guidance command
    // TODO: for now, return the desired acceleration vector
    virtual std::vector<double> guidanceCommand(const State& s, const State& target) = 0;
};

class IAutopilot
{
public:
    virtual ~IAutopilot() = default;
    // turn guidance command into actuator deflections
    virtual std::vector<double> control(const State& s, const std::vector<double>& guidanceCmd) = 0;
};

} // namespace sim