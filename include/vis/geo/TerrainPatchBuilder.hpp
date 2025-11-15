#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "geo/GeoTypes.hpp"
#include "vis/geo/GeoMapper.hpp"

namespace vis {

/**
 * @brief Configuration for a local terrain patch.
 */
struct TerrainPatchConfig {
    float halfSizeMeters = 500000.0f;  ///< Half-width in ENU (meters).
    int   resolution      = 64;       ///< Number of quads per side (N).
    float patchOffset     = 0.0f;     ///< Visual offset above the globe radius.
};

/**
 * @brief Build a terrain patch mesh around a geo origin on the visual globe.
 *
 * Fills positions, texcoords, normals, and indices.
 */
void buildTerrainPatchMesh(const geo::GeoOrigin& origin,
                           const GeoMapper&      mapper,
                           const TerrainPatchConfig& cfg,
                           std::vector<float>&   positions,
                           std::vector<float>&   texcoords,
                           std::vector<float>&   normals,
                           std::vector<unsigned int>& indices);

} // namespace vis
