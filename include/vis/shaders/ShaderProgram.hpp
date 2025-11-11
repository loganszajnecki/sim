#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glad/glad.h>

namespace vis {

/**
 * @brief RAII wrapper around an OpenGL shader program.
 *
 * Responsibilities:
 *  - Load vertex and fragment shader source from disk.
 *  - Compile individual shaders and attach them to a program.
 *  - Provide helpers to bind attributes and set uniforms.
 *
 * Lifetime / RAII:
 *  - Constructor compiles shaders and creates a program (but does not link).
 *  - Derived classes (e.g. LineShader) typically:
 *      1) call bindAttributes()
 *      2) call glLinkProgram(program_) and glValidateProgram(program_)
 *      3) call getAllUniformLocations()
 *  - Destructor calls destroy(), which:
 *      - detaches & deletes shaders
 *      - deletes the program
 *
 * Requirements:
 *  - All methods that touch GL (constructor, start/stop, destroy, uniform
 *    setters) must only be called when a valid OpenGL context is current.
 */
class ShaderProgram 
{
public:
    ShaderProgram(const std::string& vertPath,
                  const std::string& fragPath);
    virtual ~ShaderProgram();

    ShaderProgram(const ShaderProgram&)            = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /// Bind this shader program (glUseProgram).
    void start() const;

    /// Unbind any shader program (glUseProgram(0)).
    void stop() const;

    /// Explicit teardown of GL resources; safe and idempotent.
    void destroy();

protected:
    /// Implemented by derived shaders to bind attribute locations.
    virtual void bindAttributes() = 0;

    /// Implemented by derived shaders to fetch uniform locations.
    virtual void getAllUniformLocations() = 0;

    /// Look up the location of a uniform by name (may return -1).
    GLint getUniformLocation(const char* name) const;

    // Uniform helpers (call only after start()).
    void loadFloat(GLint location, float v) const;
    void loadInt(GLint location, int v) const;
    void loadBool(GLint location, bool v) const;
    void loadVec3(GLint location, const glm::vec3& v) const;
    void loadMat4(GLint location, const glm::mat4& m) const;

    /// Bind a vertex attribute index to a named attribute in the program.
    void bindAttribute(GLuint index, const char* name);

protected:
    GLuint program_ = 0;
    GLuint vert_    = 0;
    GLuint frag_    = 0;

private:
    static GLuint loadShaderFromFile(const std::string& path, GLenum type);
};

} // namespace vis
