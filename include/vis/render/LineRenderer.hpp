#pragma once

#include <vector>
#include <memory>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "vis/shaders/LineShader.hpp"

namespace vis {

/**
 * @brief Handles line-based rendering (grid, axes, trails, crosses).
 *
 * RAII:
 *  - init() creates GL resources; shutdown() destroys them.
 *  - Destructor calls shutdown() (idempotent), but you must ensure a
 *    valid GL context is current when Renderer/LineRenderer are torn down.
 */
class LineRenderer {
public:
    LineRenderer() = default;
    ~LineRenderer() noexcept { shutdown(); }

    LineRenderer(const LineRenderer&)            = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;
    LineRenderer(LineRenderer&&)                 = delete;
    LineRenderer& operator=(LineRenderer&&)      = delete;

    /// Initialize shader + GL resources. Safe to call once.
    bool init(const char* vertPath,
                            const char* fragPath,
                            std::size_t trailCapacity);

    /// Destroy all GL resources (safe, idempotent).
    void shutdown();

    /// Begin a line pass: bind shader and set VP.
    void begin(const glm::mat4& vp);

    /// End a line pass.
    void end();

    /// Optional: draw world axes at origin.
    void drawAxes();

    /// Draw a trail using the dynamic VBO (LINE_STRIP).
    void drawTrail(GLuint vao,
                   GLuint vbo,
                   const std::vector<glm::vec3>& pts,
                   const glm::vec3& color);

    /// Draw a 3D cross centered at p.
    void drawCross(const glm::vec3& p,
                   float L,
                   const glm::vec3& color);

    /// Access to trail VAOs so the caller can choose missile vs target.
    GLuint missileTrailVAO() const noexcept { return vao_trail_m_; }
    GLuint missileTrailVBO() const noexcept { return vbo_trail_m_; }
    GLuint targetTrailVAO() const noexcept  { return vao_trail_t_; }
    GLuint targetTrailVBO() const noexcept  { return vbo_trail_t_; }

private:
    void initGridAxes_();
    void initDynamicVBOs_(std::size_t trailCapacity);

    bool initialized_{false};

    std::unique_ptr<LineShader> shader_;

    // Grid + axes
    GLuint vao_grid_{0}, vbo_grid_{0};
    GLuint vao_axes_{0}, vbo_axes_{0};
    GLuint count_grid_{0}, count_axes_{0};

    // Dynamic trails
    GLuint vao_trail_m_{0}, vbo_trail_m_{0};
    GLuint vao_trail_t_{0}, vbo_trail_t_{0};
    std::size_t trailCapacity_{0};
};

} // namespace vis
