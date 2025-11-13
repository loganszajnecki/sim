#include "geo/GeoUtils.hpp"
#include <cmath>

namespace {

constexpr double kDegToRad = M_PI / 180.0;
constexpr double kRadToDeg = 180.0 / M_PI;

// WGS-84 ellipsoid constants
constexpr double kA  = 6378137.0;                 // semi-major axis (m)
constexpr double kF  = 1.0 / 298.257223563;       // flattening
constexpr double kE2 = 2.0 * kF - kF * kF;        // first eccentricity^2

inline void latLonToRad(const geo::GeoLLA& lla, double& lat_rad, double& lon_rad)
{
    lat_rad = lla.lat_deg * kDegToRad;
    lon_rad = lla.lon_deg * kDegToRad;
}

} // namespace

namespace geo {

glm::dvec3 ecefFromLLA(const GeoLLA& lla)
{
    double lat, lon;
    latLonToRad(lla, lat, lon);

    const double sinLat = std::sin(lat);
    const double cosLat = std::cos(lat);
    const double cosLon = std::cos(lon);
    const double sinLon = std::sin(lon);

    const double N = kA / std::sqrt(1.0 - kE2 * sinLat * sinLat);

    const double x = (N + lla.alt_m) * cosLat * cosLon;
    const double y = (N + lla.alt_m) * cosLat * sinLon;
    const double z = (N * (1.0 - kE2) + lla.alt_m) * sinLat;

    return glm::dvec3{x, y, z};
}

GeoLLA llaFromECEF(const glm::dvec3& ecef)
{
    const double x = ecef.x;
    const double y = ecef.y;
    const double z = ecef.z;

    const double lon = std::atan2(y, x);

    const double p   = std::sqrt(x * x + y * y);
    double       lat = std::atan2(z, p * (1.0 - kE2));

    for (int i = 0; i < 5; ++i) {
        const double sinLat = std::sin(lat);
        const double N      = kA / std::sqrt(1.0 - kE2 * sinLat * sinLat);
        lat = std::atan2(z + kE2 * N * sinLat, p);
    }

    const double sinLat = std::sin(lat);
    const double N      = kA / std::sqrt(1.0 - kE2 * sinLat * sinLat);
    const double alt    = p / std::cos(lat) - N;

    GeoLLA out;
    out.lat_deg = lat * kRadToDeg;
    out.lon_deg = lon * kRadToDeg;
    out.alt_m   = alt;
    return out;
}

GeoOrigin makeOrigin(const GeoLLA& lla)
{
    GeoOrigin origin;
    origin.lla  = lla;
    origin.ecef = ecefFromLLA(lla);

    double lat, lon;
    latLonToRad(lla, lat, lon);
    const double sinLat = std::sin(lat);
    const double cosLat = std::cos(lat);
    const double sinLon = std::sin(lon);
    const double cosLon = std::cos(lon);

    // Rows of rotation from ECEF to ENU:
    //  e = [-sinLon       ,  cosLon       ,  0     ]
    //  n = [-sinLat*cosLon, -sinLat*sinLon,  cosLat]
    //  u = [ cosLat*cosLon,  cosLat*sinLon,  sinLat]
    //
    // GLM mat3 constructor takes *columns* (col0, col1, col2), so we arrange:
    //  col0 = (e.x, n.x, u.x), col1 = (e.y, n.y, u.y), col2 = (e.z, n.z, u.z)

    glm::dvec3 col0(
        -sinLon,
        -sinLat * cosLon,
         cosLat * cosLon
    );

    glm::dvec3 col1(
         cosLon,
        -sinLat * sinLon,
         cosLat * sinLon
    );

    glm::dvec3 col2(
        0.0,
        cosLat,
        sinLat
    );

    origin.ecef_to_enu = glm::dmat3(col0, col1, col2);
    origin.enu_to_ecef = glm::transpose(origin.ecef_to_enu);

    return origin;
}

glm::dvec3 enuFromECEF(const glm::dvec3& ecef, const GeoOrigin& origin)
{
    glm::dvec3 diff = ecef - origin.ecef;
    return origin.ecef_to_enu * diff;
}

glm::dvec3 ecefFromENU(const glm::dvec3& enu, const GeoOrigin& origin)
{
    glm::dvec3 diff = origin.enu_to_ecef * enu;
    return origin.ecef + diff;
}

glm::dvec3 enuFromLLA(const GeoLLA& p, const GeoOrigin& origin)
{
    return enuFromECEF(ecefFromLLA(p), origin);
}

GeoLLA llaFromENU(const glm::dvec3& enu, const GeoOrigin& origin)
{
    return llaFromECEF(ecefFromENU(enu, origin));
}

} // namespace geo
