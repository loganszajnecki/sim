#pragma once

#include <glm/glm.hpp>

#include "vis/entities/Entity.hpp"
#include "vis/entities/Light.hpp"
#include "vis/models/TexturedModel.hpp"
#include "vis/models/RawModel.hpp"
#include "vis/Loader.hpp"
#include "vis/render/MasterRenderer.hpp"
#include "vis/entities/Camera.hpp"
#include "vis/geo/GeoMapper.hpp"
#include "vis/geo/TerrainPatchBuilder.hpp"
#include "geo/GeoTypes.hpp"

namespace vis {

/**
 * @brief Holds Earth, terrain, missile, ground, and sun for the 3D world.
 *
 * RAII:
 *  - Owns entities and their models (by value).
 *  - Uses Loader (non-owning) to create GL resources.
 *  - No direct GL resource deletes here; Loader handles that.
 */
class WorldScene {
public:
    WorldScene() = default;

    /// Initialize sun, earth, missile, ground, launch marker, and terrain.
    void init(Loader& loader,
              GeoMapper& mapper,
              const geo::GeoOrigin* origin);

    /// Update dynamic state each frame (missile world-space position).
    void updateMissile(const glm::vec3& missileWorld);

    /// Submit all visible entities and call MasterRenderer::render.
    void submitGlobe(MasterRenderer& renderer, const Camera& cam);
    void submitLocal(MasterRenderer& renderer, const Camera& cam);

    /// Debug / line overlay helpers.
    const glm::vec3& launchMarkerPos() const noexcept { return launchMarkerPos_; }
    bool hasLaunchMarker() const noexcept { return haveLaunchMarkerPos_; }

    bool earthEnabled() const noexcept { return earthEnabled_; }
    bool terrainEnabled() const noexcept { return terrainEnabled_; }

private:
    void initSun_();
    void initMissile_(Loader& loader);
    void initEarth_(Loader& loader, GeoMapper& mapper, const geo::GeoOrigin* origin);
    void initLaunchMarker_(const geo::GeoOrigin* origin, GeoMapper& mapper);
    void initTerrainPatch_(Loader& loader,
                           const geo::GeoOrigin* origin,
                           GeoMapper& mapper);

    // Light.
    Light sun_;

    // Earth + marker.
    TexturedModel earthModel_{};
    Entity        earthEntity_{};
    Entity        launchMarkerEntity_{};
    bool          earthEnabled_{true};

    // Missile.
    TexturedModel missileModel_{};
    Entity        missileEntity_{};

    // Local terrain patch.
    RawModel      terrainRaw_{};
    TexturedModel terrainModel_{};
    Entity        terrainEntity_{};
    bool          terrainEnabled_{false};

    // Launch marker debug state.
    glm::vec3 launchMarkerPos_{0.0f, 0.0f, 0.0f};
    bool      haveLaunchMarkerPos_{false};
};

} // namespace vis
