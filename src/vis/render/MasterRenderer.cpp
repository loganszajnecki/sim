#include "vis/render/MasterRenderer.hpp"

#include <glad/glad.h>

namespace vis {

MasterRenderer::MasterRenderer(const Camera& cam,
                               const std::string& vertPath,
                               const std::string& fragPath)
    : shader_(vertPath, fragPath)
    , renderer_(shader_)
{
    // Load projection once from the camera.
    shader_.start();
    shader_.loadProjectionMatrix(cam);
    shader_.stop();
}

void MasterRenderer::processEntity(Entity& e)
{
    if (!e.model) {
        return;
    }
    entities_[e.model].push_back(&e);
}

void MasterRenderer::prepare()
{
    glEnable(GL_DEPTH_TEST);

    // We set the sky color here. The actual clear happens in Renderer::beginFrame(),
    // which currently also sets glClearColor and glClear(). If you'd prefer
    // MasterRenderer to own clearing entirely, you can:
    //
    //   - move glClearColor + glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    //     here, and
    //   - remove them from Renderer::beginFrame().
    //
    glClearColor(skyColor_.r, skyColor_.g, skyColor_.b, 1.0f);
}

void MasterRenderer::render(const Light& sun, const Camera& cam)
{
    if (entities_.empty()) {
        return;
    }

    prepare();

    shader_.start();
    shader_.loadSkyColor(skyColor_);
    shader_.loadLight(sun);
    shader_.loadViewMatrix(cam);

    renderer_.render(entities_);

    shader_.stop();
    entities_.clear();
}

} // namespace vis
