#include "modules/ProNav.hpp"
#include <cmath>

namespace {
inline void cross(const double a[3], const double b[3], double c[3]) {
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];
}
inline double dot(const double a[3], const double b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
inline double norm(const double a[3]) {
    return std::sqrt(dot(a,a)) + 1e-9;
}
}

namespace modules {

// 3D vector PN: a_cmd = N * Vc * (los_hat x (v_rel x los_hat)) // |r|
std::vector<double> ProNav::computeGuidanceCommand(const sim::State& m,
                                                     const sim::State& t) {
    double r[3]   = { t.x[0]-m.x[0], t.x[1]-m.x[1], t.x[2]-m.x[2] };
    double v_rel[3] = { t.x[3]-m.x[3], t.x[4]-m.x[4], t.x[5]-m.x[5] };
    const double R = norm(r);

    // unit LOS
    double los_hat[3] = { r[0]/R, r[1]/R, r[2]/R };

    // closing speed Vc = -dot(v_rel, los_hat)
    const double Vc = - (v_rel[0]*los_hat[0] + v_rel[1]*los_hat[1] + v_rel[2]*los_hat[2]);

    // a_cmd direction: los_hat × (v_rel × los_hat), magnitude scaled by N*Vc/|r|
    double v_cross_los[3]; cross(v_rel, los_hat, v_cross_los);
    double dir[3];         cross(los_hat, v_cross_los, dir);

    // scale
    double scale = (N_ * Vc) / R;
    double a_cmd[3] = { scale * dir[0], scale * dir[1], scale * dir[2] };

    // saturate accel
    double a_norm = std::sqrt(a_cmd[0]*a_cmd[0] + a_cmd[1]*a_cmd[1] + a_cmd[2]*a_cmd[2]);
    if (a_norm > amax_) {
        double s = amax_ / a_norm;
        a_cmd[0] *= s; a_cmd[1] *= s; a_cmd[2] *= s;
    }

    return { a_cmd[0], a_cmd[1], a_cmd[2] };
}
} // namespace modules