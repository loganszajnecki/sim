#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "vis/shaders/EntityShader.hpp"
#include "vis/render/EntityRenderer.hpp"
#include "vis/entities/Entity.hpp"
#include "vis/entities/Light.hpp"
#include "vis/entities/Camera.hpp"

namespace vis {

class MasterRenderer
{
public:
    MasterRenderer(const Camera& cam, const std::string& vertPath,
                   const std::string& fragPath);

    using BatchMap = EntityRenderer::BatchMap;

    void processEntity(Entity& e);
    void render(const Light& sun, const Camera& cam);
    void setSkyColor(const glm::vec3 rgb) { skyColor_ = rgb; }

private:
    void prepare();

private:
    glm::vec3 skyColor_{0.08f, 0.09f, 0.10f};
    
    EntityShader   shader_;
    EntityRenderer renderer_;
    BatchMap       entities_;
};


} // namespace vis