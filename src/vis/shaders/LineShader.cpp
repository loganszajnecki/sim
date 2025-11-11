#include "vis/shaders/LineShader.hpp"

#include <glad/glad.h> // for glLinkProgram / glValidateProgram

namespace vis {

LineShader::LineShader(const std::string& vertPath,
                       const std::string& fragPath)
    : ShaderProgram(vertPath, fragPath)
{
    // 1) Bind attribute locations before linking.
    bindAttributes();

    // 2) Link and validate program. (ShaderProgram owns program_).
    glLinkProgram(program_);
    glValidateProgram(program_);

    // NOTE: For production code, you may want to add link/validate status
    // checks in ShaderProgram and throw/log on failure.

    // 3) Query uniform locations.
    getAllUniformLocations();
}

void LineShader::bindAttributes()
{
    // Only a position attribute at location 0.
    bindAttribute(0, "position");
}

void LineShader::getAllUniformLocations()
{
    loc_vp_    = getUniformLocation("uVP");
    loc_color_ = getUniformLocation("uColor");
}

void LineShader::loadVP(const glm::mat4& vp)
{
    loadMat4(loc_vp_, vp);
}

void LineShader::loadColor(const glm::vec3& rgb)
{
    loadVec3(loc_color_, rgb);
}

} // namespace vis
