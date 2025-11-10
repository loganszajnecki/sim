#pragma once
#include "vis/shaders/ShaderProgram.hpp"
#include <glm/glm.hpp>

namespace vis {

class LineShader : public ShaderProgram
{
public:
    LineShader(const std::string& vertPath, const std::string& fragPath);

    void loadVP(const glm::mat4& vp);
    void loadColor(const glm::vec3& rgb);

protected:
    void bindAttributes() override;
    void getAllUniformLocations() override;

private:
    GLint loc_vp_    = -1;
    GLint loc_color_ = -1;
};

} // namespace vis