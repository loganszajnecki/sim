#include "vis/shaders/ShaderProgram.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "vis/shaders/ShaderProgram.hpp"

namespace vis {

static std::string readTextFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f) {
        std::cerr << "[ShaderProgram] Failed to open file: " << path << "\n";
        return {};
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint ShaderProgram::loadShaderFromFile(const std::string& path, GLenum type)
{
    std::string src = readTextFile(path);
    if (src.empty()) {
        std::cerr << "[ShaderProgram] Empty shader source: " << path << "\n";
    }

    GLuint id = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(id, 1, &csrc, nullptr);
    glCompileShader(id);

    GLint status = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint logLen = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(id, logLen, nullptr, log.data());
        std::cerr << "[ShaderProgram] Compile error in " << path << ":\n"
                  << log << "\n";
    }

    return id;
}

ShaderProgram::ShaderProgram(const std::string &vertPath, const std::string &fragPath)
{
    vert_ = loadShaderFromFile(vertPath, GL_VERTEX_SHADER);
    frag_ = loadShaderFromFile(fragPath, GL_FRAGMENT_SHADER);

    program_ = glCreateProgram();
    glAttachShader(program_, vert_);
    glAttachShader(program_, frag_);
}

ShaderProgram::~ShaderProgram()
{
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept 
{
    *this = std::move(other);
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        destroy();
        program_ = other.program_;
        vert_    = other.vert_;
        frag_    = other.frag_;
        other.program_ = 0;
        other.vert_    = 0;
        other.frag_    = 0;
    }
    return *this;
}

void ShaderProgram::start() const {
    glUseProgram(program_);
}


void ShaderProgram::stop() const {
    glUseProgram(0);
}

void ShaderProgram::destroy() {
    if (program_ != 0) {
        glUseProgram(0);
        if (vert_ != 0) {
            glDetachShader(program_, vert_);
            glDeleteShader(vert_);
        }
        if (frag_ != 0) {
            glDetachShader(program_, frag_);
            glDeleteShader(frag_);
        }
        glDeleteProgram(program_);
    }
    program_ = 0;
    vert_    = 0;
    frag_    = 0;
}


GLint ShaderProgram::getUniformLocation(const char* name) const {
    return glGetUniformLocation(program_, name);
}

void ShaderProgram::bindAttribute(GLuint index, const char* name) {
    glBindAttribLocation(program_, index, name);
}

// uniform helpers
void ShaderProgram::loadFloat(GLint location, float v) const {
    glUniform1f(location, v);
}

void ShaderProgram::loadInt(GLint location, int v) const {
    glUniform1i(location, v);
}

void ShaderProgram::loadBool(GLint location, bool v) const {
    glUniform1f(location, v ? 1.0f : 0.0f);
}

void ShaderProgram::loadVec3(GLint location, const glm::vec3& v) const {
    glUniform3f(location, v.x, v.y, v.z);
}

void ShaderProgram::loadMat4(GLint location, const glm::mat4& m) const {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

} // namespace vis
