#pragma once
#include <cstdint>

namespace vis {

// Minimal DTO; will expand in Phase 3/4
struct TelemetrySample {
    double t{0.0};
    float mx{0}, my{0}, mz{0};
    float mvx{0}, mvy{0}, mvz{0};
    float tx{0}, ty{0}, tz{0};
    float ax{0}, ay{0}, az{0};
};

} // namespace vis
