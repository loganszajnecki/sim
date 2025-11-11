#pragma once

#include <glad/glad.h>

namespace vis {

/**
 * @brief Simple material/texture descriptor for a model.
 *
 * Fields:
 *  - id              : OpenGL texture object name (GLuint).
 *  - shineDamper     : Controls specular highlight spread
 *                      (higher = tighter highlight).
 *  - reflectivity    : Controls specular intensity.
 *  - hasTransparency : When true, back-face culling is disabled for this
 *                      model to avoid clipping transparent surfaces.
 *  - useFakeLighting : When true, lighting is computed without normals
 *                      (e.g., for billboards or unlit geometry).
 *
 * Lifetime:
 *  - This struct does not manage the lifetime of the GL texture.
 *  - The owner (Loader/Renderer) is responsible for calling glDeleteTextures.
 */
struct ModelTexture {
    GLuint id = 0;

    float shineDamper     = 1.0f;
    float reflectivity    = 0.0f;
    bool  hasTransparency = false;
    bool  useFakeLighting = false;

    ModelTexture() = default;
    explicit ModelTexture(GLuint texId) : id(texId) {}
};

} // namespace vis
