#pragma once

#include "sim/Components.hpp"

namespace modules {

class PurePursuitGuidance : public sim::IGuidance
{
public:
    PurePursuitGuidance(double gain = 1.0);
    std::vector<double> guidanceCommand(const sim::State& s, const sim::State& target) override;
private:
    double gain_;
};

} // namespace modules