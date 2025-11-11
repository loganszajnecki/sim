// SimpleAccelAutopilot.cpp
#include "sim/models/SimpleAccelAutopilot.hpp"
#include <algorithm> // std::clamp

namespace models {

SimpleAccelAutopilot::SimpleAccelAutopilot(double gain, double maxCmd)
    : gain_(gain)
    , maxCmd_(maxCmd)
{
}

std::vector<double> SimpleAccelAutopilot::computeActuatorCommand(
    const sim::State& /*state*/,
    const std::vector<double>& desiredAccel)
{
    // Output is always 3 elements: [ux, uy, uz].
    std::vector<double> out(3, 0.0);

    // Assume desiredAccel is [ax, ay, az]. If fewer than 3 elements are
    // provided, the remaining outputs stay at zero.
    const std::size_t n = std::min<std::size_t>(3, desiredAccel.size());
    for (std::size_t i = 0; i < n; ++i) {
        const double u = gain_ * desiredAccel[i];
        out[i] = std::clamp(u, -maxCmd_, maxCmd_);
    }

    return out;
}

} // namespace models
