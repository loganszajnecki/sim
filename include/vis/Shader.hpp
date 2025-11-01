#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace vis {

class Shader
{
public:
    Shader() = default;
    ~Shader();

    bool compile(const char* vs_src, const char* fs_src);
    void use() const { glUseProgram(prog_); }
    GLuint id() const { return prog_; }

    // uniform helpers
    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;
private:
    GLuint prog_ = 0;
};

} // namespace vis