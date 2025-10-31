#pragma once
#include "sim/Components.hpp"
#include <algorithm>

namespace modules {

class ProNav : public sim::IGuidance
{
public:
    explicit ProNav(double N = 3.0, double amax = 100.0) : N_(N), amax_(amax) {}
    std::vector<double>guidanceCommand(const sim::State& m, const sim::State& t) override;
private:
    double N_;
    double amax_;
};

} // namespace modules