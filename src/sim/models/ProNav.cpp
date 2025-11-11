// ProNav.cpp
#include "sim/models/ProNav.hpp"
#include <cmath>
#include <cassert>

namespace {

// Simple 3D vector helpers on raw arrays.
inline void cross(const double a[3], const double b[3], double c[3]) {
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
}

inline double dot(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline double norm(const double a[3]) {
    return std::sqrt(dot(a, a));
}

} // anonymous namespace

namespace models {

// 3D vector PN: a_cmd = (N * Vc / |r|) * (los_hat × (v_rel × los_hat))
std::vector<double> ProNav::computeGuidanceCommand(const sim::State& m,
                                                   const sim::State& t)
{
    // Require at least 3-DoF layout: pos(3), vel(3).
    assert(m.size() >= VZ_IDX + 1);
    assert(t.size() >= VZ_IDX + 1);

    // Relative position r = target - missile.
    double r[3] = {
        t.x[PX_IDX] - m.x[PX_IDX],
        t.x[PY_IDX] - m.x[PY_IDX],
        t.x[PZ_IDX] - m.x[PZ_IDX]
    };

    // Relative velocity v_rel = target - missile.
    double v_rel[3] = {
        t.x[VX_IDX] - m.x[VX_IDX],
        t.x[VY_IDX] - m.x[VY_IDX],
        t.x[VZ_IDX] - m.x[VZ_IDX]
    };

    const double R = norm(r);
    constexpr double kMinRange = 1e-3; // [m] small guard to avoid division by ~0

    // If the range is extremely small (or zero), the PN command is undefined;
    // return zero to avoid numerical issues.
    if (R < kMinRange) {
        return {0.0, 0.0, 0.0};
    }

    // Unit line-of-sight vector.
    double los_hat[3] = { r[0] / R, r[1] / R, r[2] / R };

    // Closing speed Vc = -dot(v_rel, los_hat).
    const double Vc = -dot(v_rel, los_hat);

    // Direction term: los_hat × (v_rel × los_hat).
    double v_cross_los[3];
    cross(v_rel, los_hat, v_cross_los);

    double dir[3];
    cross(los_hat, v_cross_los, dir);

    // Scale factor: N * Vc / |r|.
    const double scale = (N_ * Vc) / R;

    double a_cmd[3] = {
        scale * dir[0],
        scale * dir[1],
        scale * dir[2]
    };

    // Saturate acceleration magnitude to amax_.
    const double a_norm = norm(a_cmd);
    if (a_norm > amax_ && a_norm > 0.0) {
        const double s = amax_ / a_norm;
        a_cmd[0] *= s;
        a_cmd[1] *= s;
        a_cmd[2] *= s;
    }

    return { a_cmd[0], a_cmd[1], a_cmd[2] };
}

} // namespace models
