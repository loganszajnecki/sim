#pragma once
#include <glad/glad.h>

namespace vis {

struct ModelTexture {
    GLuint id = 0;

    float shineDamper   = 1.0f;
    float reflectivity  = 0.0f;
    bool  hasTransparency = false;
    bool  useFakeLighting = false;

    ModelTexture() = default;
    explicit ModelTexture(GLuint texId) : id(texId) {}
};

} // namespace vis
