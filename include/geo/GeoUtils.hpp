#pragma once

#include "geo/GeoTypes.hpp"

namespace geo {

// Create a GeoOrigin from a geodetic launch site
GeoOrigin makeOrigin(const GeoLLA& lla);

// Core transforms
glm::dvec3 ecefFromLLA(const GeoLLA& lla);
GeoLLA     llaFromECEF(const glm::dvec3& ecef);

// Local ENU around origin
glm::dvec3 enuFromECEF(const glm::dvec3& ecef, const GeoOrigin& origin);
glm::dvec3 ecefFromENU(const glm::dvec3& enu, const GeoOrigin& origin);

// Convenience
glm::dvec3 enuFromLLA(const GeoLLA& p, const GeoOrigin& origin);
GeoLLA     llaFromENU(const glm::dvec3& enu, const GeoOrigin& origin);

} // namespace geo