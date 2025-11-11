#pragma once
#include <glad/glad.h>

namespace vis {

struct RawModel {
    GLuint vao = 0;
    GLsizei indexCount = 0;

    RawModel() = default;
    RawModel(GLuint vao_, GLsizei count_) : vao(vao_), indexCount(count_) {}
};

} // namespace vis