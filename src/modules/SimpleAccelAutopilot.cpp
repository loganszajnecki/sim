#include "modules/SimpleAccelAutopilot.hpp"
#include <algorithm> // std::clamp

namespace modules {

SimpleAccelAutopilot::SimpleAccelAutopilot(double gain, double maxCmd)
    : gain_(gain), maxCmd_(maxCmd) {}

std::vector<double> SimpleAccelAutopilot::computeActuatorCommand(
    const sim::State& /*s*/, 
    const std::vector<double> &desiredAccel)
{
    std::vector<double> out(3, 0.0);

    // Assume desiredAccel is [ax, ay, az]
    for (size_t i = 0; i < 3 && i < desiredAccel.size(); ++i) {
        double u = gain_ * desiredAccel[i];
        out[i] = std::clamp(u, -maxCmd_, maxCmd_);
    }

    return out;
}

} // namespace modules