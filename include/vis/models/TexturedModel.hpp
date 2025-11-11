#pragma once

#include "vis/models/RawModel.hpp"
#include "vis/models/ModelTexture.hpp"

namespace vis {

/**
 * @brief Pair of mesh data (RawModel) and material/texture data (ModelTexture).
 *
 * A TexturedModel represents a *shared* renderable asset:
 *  - RawModel holds the VAO and index count for the mesh.
 *  - ModelTexture holds the OpenGL texture ID and material params.
 *
 * Multiple Entity instances can reference the same TexturedModel to
 * draw many copies of the same mesh with the same texture.
 *
 * Lifetime:
 *  - TexturedModel does not own the GL resources directly; those are
 *    created and deleted via Loader and ShaderProgram/Renderer.
 *  - It is typically a value type that is constructed once and then
 *    referenced by pointer from Entity.
 */
struct TexturedModel {
    RawModel     raw;
    ModelTexture texture;

    TexturedModel() = default;

    TexturedModel(const RawModel& r,
                  const ModelTexture& t)
        : raw(r), texture(t) {}
};

} // namespace vis
