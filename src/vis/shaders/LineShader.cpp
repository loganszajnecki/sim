#include "vis/shaders/LineShader.hpp"

namespace vis {

LineShader::LineShader(const std::string& vertPath,
                       const std::string& fragPath)
    : ShaderProgram(vertPath, fragPath)
{
    // 1) bind attributes
    bindAttributes();
    // 2) link/validate program (ShaderProgram exposes program_ for this)
    glLinkProgram(program_);
    glValidateProgram(program_);
    // 3) query uniforms
    getAllUniformLocations();
}

void LineShader::bindAttributes() {
    // only position at location 0
    bindAttribute(0, "position");
}

void LineShader::getAllUniformLocations() {
    loc_vp_    = getUniformLocation("uVP");
    loc_color_ = getUniformLocation("uColor");
}

void LineShader::loadVP(const glm::mat4& vp) {
    loadMat4(loc_vp_, vp);
}

void LineShader::loadColor(const glm::vec3& rgb) {
    loadVec3(loc_color_, rgb);
}

} // namespace vis
