#include "modules/PurePursuitGuidance.hpp"
#include <cmath>

namespace modules {

PurePursuitGuidance::PurePursuitGuidance(double gain) : gain_(gain) {}

std::vector<double> PurePursuitGuidance::guidanceCommand(const sim::State &s, const sim::State &target)
{
    // simple vector from missile position to target position scaled by gain
    std::vector<double> cmd(3, 0.0);
    if (s.size() >= 3 && target.size() >= 3) {
        cmd[0] = gain_ * (target.x[0] - s.x[0]);
        cmd[1] = gain_ * (target.x[1] - s.x[1]);
        cmd[2] = gain_ * (target.x[2] - s.x[2]);
    }
    return cmd;
}

} // namespace modules