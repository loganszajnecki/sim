#include "vis/geo/TerrainPatchBuilder.hpp"

namespace vis {

void buildTerrainPatchMesh(const geo::GeoOrigin& origin,
                           const GeoMapper&      mapper,
                           const TerrainPatchConfig& cfg,
                           std::vector<float>&   positions,
                           std::vector<float>&   texcoords,
                           std::vector<float>&   normals,
                           std::vector<unsigned int>& indices)
{
    positions.clear();
    texcoords.clear();
    normals.clear();
    indices.clear();

    const int   N         = cfg.resolution;
    const float HALF_SIZE = cfg.halfSizeMeters;
    const float patchOffset = cfg.patchOffset;

    const int VERTS_X = N + 1;
    const int VERTS_Y = N + 1;

    positions.reserve(VERTS_X * VERTS_Y * 3);
    texcoords.reserve(VERTS_X * VERTS_Y * 2);
    normals.reserve(VERTS_X * VERTS_Y * 3);
    indices.reserve(N * N * 6);

    for (int j = 0; j < VERTS_Y; ++j) {
        float v    = static_cast<float>(j) / static_cast<float>(N);
        float yENU = (v * 2.0f - 1.0f) * HALF_SIZE; // north

        for (int i = 0; i < VERTS_X; ++i) {
            float u    = static_cast<float>(i) / static_cast<float>(N);
            float xENU = (u * 2.0f - 1.0f) * HALF_SIZE; // east

            // Flat patch in ENU at z=0 (we’ll add real height later).
            glm::vec3 enu(static_cast<float>(xENU),
                          static_cast<float>(yENU),
                          0.0f);

            // Project onto visual Earth sphere (mapper encapsulates Earth radius).
            glm::vec3 pos = mapper.enuToGlobe(enu, patchOffset);

            // Positions.
            positions.push_back(pos.x);
            positions.push_back(pos.y);
            positions.push_back(pos.z);

            // UVs: simple [0,1] over the patch.
            texcoords.push_back(u);
            texcoords.push_back(v);

            // Normals ≈ radial from Earth center (good enough for lighting).
            glm::vec3 dir = glm::normalize(pos); // since pos is already at radius+offset
            normals.push_back(dir.x);
            normals.push_back(dir.y);
            normals.push_back(dir.z);
        }
    }

    // Build indices for a regular grid (CCW as seen from outside the Earth).
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            int i0 = j * VERTS_X + i;
            int i1 = i0 + 1;
            int i2 = i0 + VERTS_X;
            int i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }
}

} // namespace vis
