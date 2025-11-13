#pragma once

#include <glm/glm.hpp>

namespace geo {

// Geodetic coordinates (WGS-84)
struct GeoLLA {
    double lat_deg = 0.0; // latitude (degrees, positive north)
    double lon_deg = 0.0; // longitude (degrees, positive east)
    double alt_m   = 0.0; // altitude above ellipsoid (meters)
};

// Precomputed origin info for local ENU frame
struct GeoOrigin {
    GeoLLA      lla;          // geodetic origin
    glm::dvec3  ecef;         // origin in ECEF (meters)
    glm::dmat3  ecef_to_enu;  // rotation: ECEF vec -> ENU vec
    glm::dmat3  enu_to_ecef;  // rotation: ENU vec -> ECEF vec
};

} // namespace geo