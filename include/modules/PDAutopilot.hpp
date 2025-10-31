#pragma once

#include "sim/Components.hpp"

namespace modules {

class PDAutopilot : public sim::IAutopilot
{
public:
    PDAutopilot(double kp = 1.0, double kd = 1.0);
    std::vector<double> control(const sim::State& s, const std::vector<double>& guidanceCmd) override;
private:
    double kp_;
    double kd_;
    // store previous guidance request for derivative approx.
    std::vector<double> prevCmd_ = {0.0, 0.0, 0.0};
};

} // namespace modules