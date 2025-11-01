#include "vis/Shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

namespace vis {

static GLuint compile_stage(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "[Shader] compile error: %s\n", log.c_str());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::compile(const char* vs_src, const char* fs_src) {
    GLuint vs = compile_stage(GL_VERTEX_SHADER, vs_src);
    if (!vs) return false;
    GLuint fs = compile_stage(GL_FRAGMENT_SHADER, fs_src);
    if (!fs) {
        glDeleteShader(vs);        
        return false;
    }

    prog_ = glCreateProgram();
    glAttachShader(prog_, vs);
    glAttachShader(prog_, fs);
    glLinkProgram(prog_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok=0; 
    glGetProgramiv(prog_, GL_LINK_STATUS, &ok);
    if(!ok){
        GLint len=0; 
        glGetProgramiv(prog_, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0'); 
        glGetProgramInfoLog(prog_, len, nullptr, log.data());
        std::fprintf(stderr,"[Shader] link error: %s\n", log.c_str());
        glDeleteProgram(prog_); 
        prog_=0; 
        return false;
    }
    return true;
}

void Shader::destroy() {
    if (prog_) {
        glDeleteProgram(prog_);
        prog_ = 0;
    }
}

void Shader::setMat4(const char* name, const glm::mat4& m) const {
    GLint loc = glGetUniformLocation(prog_, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}
void Shader::setVec3(const char* name, const glm::vec3& v) const {
    GLint loc = glGetUniformLocation(prog_, name);
    glUniform3fv(loc, 1, glm::value_ptr(v));
}


} // namespace vis