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

/**
 * @brief High-level orchestration for entity-based rendering.
 *
 * Responsibilities:
 *  - Owns an EntityShader and an EntityRenderer.
 *  - Collects entities into per-model batches each frame.
 *  - Sets up common shader state (sky color, light, view matrix).
 *  - Invokes the low-level EntityRenderer draw call.
 *
 * Lifetime / usage pattern:
 *
 *   MasterRenderer renderer(cam, "entity.vert", "entity.frag");
 *
 *   // Per frame:
 *   renderer.processEntity(missileEntity);
 *   renderer.processEntity(groundEntity);
 *   renderer.render(sunLight, cam);
 *
 * Notes:
 *  - MasterRenderer does not own the Entity instances; it only stores
 *    non-owning pointers in its internal batch map.
 *    The caller must ensure entities outlive the render() call.
 *  - The projection matrix is loaded once in the constructor from the
 *    provided Camera. If the projection changes (FOV/aspect/near/far)
 *    at runtime, you should reload it via shader_.loadProjectionMatrix().
 */
class MasterRenderer
{
public:
    using BatchMap = EntityRenderer::BatchMap;

    /**
     * @brief Construct master renderer with a camera and shader paths.
     *
     * @param cam       Camera whose projection matrix is used initially.
     * @param vertPath  Vertex shader file path.
     * @param fragPath  Fragment shader file path.
     */
    MasterRenderer(const Camera& cam,
                   const std::string& vertPath,
                   const std::string& fragPath);

    MasterRenderer(const MasterRenderer&)            = delete;
    MasterRenderer& operator=(const MasterRenderer&) = delete;
    MasterRenderer(MasterRenderer&&)                 = default;
    MasterRenderer& operator=(MasterRenderer&&)      = default;

    /// Queue an entity for rendering this frame (non-owning).
    void processEntity(Entity& e);

    /**
     * @brief Render all queued entities with the given light and camera.
     *
     * After this call, the internal batch map is cleared.
     * Must be called with a valid OpenGL context current.
     */
    void render(const Light& sun, const Camera& cam);

    /// Set sky/background color used for clear and shader uniform.
    void setSkyColor(const glm::vec3 rgb) { skyColor_ = rgb; }

private:
    /// Prepare GL state for rendering (depth test, clear color, etc.).
    void prepare();

private:
    glm::vec3      skyColor_{0.08f, 0.09f, 0.10f};

    EntityShader   shader_;
    EntityRenderer renderer_;
    BatchMap       entities_;
};

} // namespace vis
