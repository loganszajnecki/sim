#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace vis {

class ShaderProgram 
{
public:
    ShaderProgram(const std::string& vertPath, const std::string& fragPath);
    virtual ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void start() const;
    void stop() const;

    void destroy(); // explicit teardown

protected:
    virtual void bindAttributes() = 0;
    virtual void getAllUniformLocations() = 0;

    GLint getUniformLocation(const char* name) const;

    // uniform helpers
    void loadFloat(GLint location, float v) const;
    void loadInt(GLint location, int v) const;
    void loadBool(GLint location, bool v) const;
    void loadVec3(GLint location, const glm::vec3& v) const;
    void loadMat4(GLint location, const glm::mat4& m) const;

    void bindAttribute(GLuint index, const char* name);

protected:
    GLuint program_ = 0;
    GLuint vert_    = 0;
    GLuint frag_    = 0;

private:
    static GLuint loadShaderFromFile(const std::string& path, GLenum type);
};

} // namespace vis