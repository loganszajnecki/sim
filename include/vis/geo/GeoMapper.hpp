#pragma once

#include <glm/glm.hpp>
#include "geo/GeoTypes.hpp"

namespace vis {

/**
 * @brief Maps local ENU coordinates around a geo origin to a visual globe.
 *
 * RAII:
 *  - Non-owning pointer to geo::GeoOrigin (must outlive this).
 *  - Owns only simple POD state (earth radius).
 */
class GeoMapper {
public:
    GeoMapper() = default;

    /// Attach a non-owning origin (may be nullptr).
    void setOrigin(const geo::GeoOrigin* origin) { origin_ = origin; }

    /// Set the visual Earth radius in world units.
    void setEarthRadius(float r) { earthWorldRadius_ = r; }

    /// Set the physics Earth radius in meters.
    void setPhysicalEarthRadius(double rMeters) { earthRadiusMeters_ = rMeters; }

    const geo::GeoOrigin* origin() const noexcept { return origin_; }
    float earthRadius() const noexcept { return earthWorldRadius_; }

    /// Returns true if we have an origin and a positive radius.
    bool isReady() const noexcept {
        return origin_ != nullptr && earthWorldRadius_ > 0.0f;
    }

    void configureFromOrigin(const geo::GeoOrigin* origin,
                             float earthWorldRadius);

    float earthWorldRadius() const noexcept { return earthWorldRadius_; }

    /**
     * @brief Map an ENU point to the visual globe surface.
     *
     * If not ready (no origin), returns enuLocal unchanged.
     *
     * @param enuLocal     Local ENU coordinates (meters).
     * @param extraOffset  Optional extra offset above the sphere (for patches, markers).
     */
    glm::vec3 enuToGlobe(const glm::vec3& enuLocal,
                         float extraOffset = 0.0f) const;
private:
    const geo::GeoOrigin* origin_{nullptr};
    float  earthWorldRadius_{0.0f};   // world units
    double earthRadiusMeters_{0.0};   // |origin_->ecef|
};

} // namespace vis
