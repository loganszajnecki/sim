#include "vis/geo/GeoMapper.hpp"

#include <glm/gtc/type_ptr.hpp>
#include "geo/GeoUtils.hpp"

namespace vis {

glm::vec3 GeoMapper::enuToGlobe(const glm::vec3& enuLocal,
                                float extraOffset) const
{
    if (!isReady()) {
        // Fall back to local coordinates if mapping not ready.
        return enuLocal;
    }

    // 1) Use horizontal ENU (x,y,0) only to determine direction on the globe.
    glm::dvec3 enuHoriz(enuLocal.x, enuLocal.y, 0.0);
    glm::dvec3 ecefHoriz = origin_->ecef + origin_->enu_to_ecef * enuHoriz;

    glm::vec3 dir = glm::normalize(glm::vec3(ecefHoriz));

    // 2) Treat ENU.z as *the* altitude above local ground (meters).
    double altMeters = static_cast<double>(enuLocal.z);

    // 3) Map meters -> world units.
    float worldUnitsPerMeter = earthWorldRadius_ /
                               static_cast<float>(earthRadiusMeters_);
    float altWorld = static_cast<float>(altMeters) * worldUnitsPerMeter;

    // Base radius is the globe radius defined at the origin.
    float radiusWorld = earthWorldRadius_ + altWorld + extraOffset;

    return dir * radiusWorld;
}

void GeoMapper::configureFromOrigin(const geo::GeoOrigin* origin,
                             float earthWorldRadius)
{
    origin_           = origin;
    earthWorldRadius_ = earthWorldRadius;

    if (origin_) {
        // Physical radius at the origin (Re + h0)
        earthRadiusMeters_ = glm::length(origin_->ecef);
    } else {
        earthRadiusMeters_ = 0.0;
    }
}

} // namespace vis
