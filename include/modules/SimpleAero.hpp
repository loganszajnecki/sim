#pragma once

#include "sim/Components.hpp"

namespace modules {

class SimpleAero : public sim::IAerodynamics
{
public:
    SimpleAero(double accGain = 1.0, double damping = 0.0, double thrustAcc = 0.0);
    sim::AeroForces compute(const sim::State& s, const std::vector<double>& actuators) override;
private:
    double gain_;
    double damping_;
    double thrustAcc_;
};

} // namespace modules