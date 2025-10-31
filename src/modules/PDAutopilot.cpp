#include "modules/PDAutopilot.hpp"

namespace modules {

PDAutopilot::PDAutopilot(double kp, double kd) : kp_(kp), kd_(kd) {}

std::vector<double> PDAutopilot::control(const sim::State& /*s*/, const std::vector<double> &guidanceCmd)
{
    std::vector<double> out(3, 0.0);
    for (size_t i = 0; i < 3; ++i) {
        double err = guidanceCmd[i];
        double derr = guidanceCmd[i] - prevCmd_[i];
        out[i] = kp_ * err + kd_ * derr;
        prevCmd_[i] = guidanceCmd[i];
    }
    return out;
}

} // namespace modules