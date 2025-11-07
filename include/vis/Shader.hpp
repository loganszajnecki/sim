#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace vis {

class Shader
{
public:
    Shader() = default;
    ~Shader() = default;
    
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept { *this = std::move(other); }
    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            prog_ = other.prog_;
            other.prog_ = 0;
        }
        return *this;
    }

    bool compile(const char* vs_src, const char* fs_src);
    void use() const { glUseProgram(prog_); }
    GLuint id() const { return prog_; }

    void destroy();

    // uniform helpers
    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setFloat(const char* name, float v) const;
    void setInt(const char* name, int v) const;
private:
    GLuint prog_ = 0;
};

} // namespace vis