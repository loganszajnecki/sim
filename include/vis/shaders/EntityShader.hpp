#pragma once

#include "vis/shaders/ShaderProgram.hpp"
#include <glm/glm.hpp>

namespace vis {

class Camera; // forward declare
struct Light; // forward declare

class EntityShader : public ShaderProgram
{
public:
    // For now, external GLSL files
    EntityShader(const std::string& vertPath, const std::string& fragPath);
    
    // uniform loaders
    void loadSkyColor(const glm::vec3& rgb);
    void loadFakeLighting(bool useFake);
    void loadShine(float damper, float reflectivity);
    void loadLight(const Light& light);
    void loadTransformation(const glm::mat4& m);
    void loadViewMatrix(const Camera& cam);
    void loadProjectionMatrix(const Camera& cam);

protected:
    void bindAttributes() override;
    void getAllUniformLocations() override;

private:
    GLint loc_transformation_     = -1;
    GLint loc_projection_         = -1;
    GLint loc_view_               = -1;
    GLint loc_lightPos_           = -1;
    GLint loc_lightColor_         = -1;
    GLint loc_shineDamper_        = -1;
    GLint loc_reflectivity_       = -1;
    GLint loc_useFakeLighting_    = -1;
    GLint loc_skyColor_           = -1;
    GLint location_textureSampler = -1;
};

} // namespace vis