#include "vis/render/LineRenderer.hpp"

#include <cstdio> // for debug if needed

namespace vis {

bool LineRenderer::init(const char* vertPath,
                        const char* fragPath,
                        std::size_t trailCapacity)
{
    if (initialized_) return true;

    // Compile the line shader.
    shader_ = std::make_unique<LineShader>(vertPath, fragPath);

    // Grid + axes.
    initGridAxes_();

    // Dynamic VBOs for trails.
    initDynamicVBOs_(trailCapacity);

    initialized_ = true;
    return true;
}

void LineRenderer::shutdown()
{
    if (!initialized_) return;
    initialized_ = false;

    // Delete GL resources.
    if (vao_grid_) { glDeleteVertexArrays(1, &vao_grid_); vao_grid_ = 0; }
    if (vbo_grid_) { glDeleteBuffers(1, &vbo_grid_);      vbo_grid_ = 0; }
    count_grid_ = 0;

    if (vao_axes_) { glDeleteVertexArrays(1, &vao_axes_); vao_axes_ = 0; }
    if (vbo_axes_) { glDeleteBuffers(1, &vbo_axes_);      vbo_axes_ = 0; }
    count_axes_ = 0;

    if (vao_trail_m_) { glDeleteVertexArrays(1, &vao_trail_m_); vao_trail_m_ = 0; }
    if (vbo_trail_m_) { glDeleteBuffers(1, &vbo_trail_m_);      vbo_trail_m_ = 0; }

    if (vao_trail_t_) { glDeleteVertexArrays(1, &vao_trail_t_); vao_trail_t_ = 0; }
    if (vbo_trail_t_) { glDeleteBuffers(1, &vbo_trail_t_);      vbo_trail_t_ = 0; }

    shader_.reset();
}

void LineRenderer::begin(const glm::mat4& vp)
{
    if (!initialized_ || !shader_) return;
    shader_->start();
    shader_->loadVP(vp);
}

void LineRenderer::end()
{
    if (!initialized_ || !shader_) return;
    shader_->stop();
}

void LineRenderer::drawAxes()
{
    if (!initialized_ || !shader_ || !vao_axes_) return;

    // Simple: draw axes in white; you can change if needed.
    shader_->loadColor(glm::vec3(1.0f, 1.0f, 1.0f));

    glBindVertexArray(vao_axes_);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(count_axes_));
    glBindVertexArray(0);
}

void LineRenderer::drawTrail(GLuint vao,
                             GLuint vbo,
                             const std::vector<glm::vec3>& pts,
                             const glm::vec3& color)
{
    if (!initialized_ || !shader_ || pts.empty()) return;

    shader_->loadColor(color);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    pts.size() * sizeof(glm::vec3),
                    pts.data());

    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(pts.size()));
    glBindVertexArray(0);
}

void LineRenderer::drawCross(const glm::vec3& p,
                             float L,
                             const glm::vec3& color)
{
    if (!initialized_ || !shader_) return;

    glm::vec3 lines[] = {
        {p.x - L, p.y,     p.z}, {p.x + L, p.y,     p.z}, // X
        {p.x,     p.y - L, p.z}, {p.x,     p.y + L, p.z}, // Y
        {p.x,     p.y,     p.z - L}, {p.x, p.y, p.z + L}  // Z
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(lines),
                 lines,
                 GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), (void*)0);

    shader_->loadColor(color);
    glDrawArrays(GL_LINES, 0, 6);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// ------------------ private helpers ------------------

// Z-up: grid on the XY plane (Z = 0), axes: X=red, Y=green, Z=blue.
void LineRenderer::initGridAxes_()
{
    // Grid (2 km x 2 km, 10 m spacing). (Matches your current logic.)
    const int   half = 1000;
    const float step = 500.f;
    std::vector<glm::vec3> grid;
    grid.reserve((half * 2 + 1) * 4);

    // Vertical lines (parallel to Y) at each X.
    for (int i = -half; i <= half; ++i) {
        const float x = i * step;
        grid.emplace_back(x, -half * step, 0.f);
        grid.emplace_back(x, +half * step, 0.f);
    }

    // Horizontal lines (parallel to X) at each Y.
    for (int i = -half; i <= half; ++i) {
        const float y = i * step;
        grid.emplace_back(-half * step, y, 0.f);
        grid.emplace_back(+half * step, y, 0.f);
    }

    count_grid_ = static_cast<GLuint>(grid.size());

    glGenVertexArrays(1, &vao_grid_);
    glGenBuffers(1, &vbo_grid_);
    glBindVertexArray(vao_grid_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_grid_);
    glBufferData(GL_ARRAY_BUFFER,
                 grid.size() * sizeof(glm::vec3),
                 grid.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), (void*)0);

    // Axes (length 500 m).
    const float L = 500.f;

    glm::vec3 axes[] = {
        {0.f, 0.f, 0.f}, {L,   0.f, 0.f},  // X
        {0.f, 0.f, 0.f}, {0.f, L,   0.f},  // Y
        {0.f, 0.f, 0.f}, {0.f, 0.f, L  },  // Z
    };
    count_axes_ = 6;

    glGenVertexArrays(1, &vao_axes_);
    glGenBuffers(1, &vbo_axes_);
    glBindVertexArray(vao_axes_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_axes_);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(axes),
                 axes,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), (void*)0);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void LineRenderer::initDynamicVBOs_(std::size_t trailCapacity)
{
    trailCapacity_ = trailCapacity;

    glGenVertexArrays(1, &vao_trail_m_);
    glGenBuffers(1, &vbo_trail_m_);
    glBindVertexArray(vao_trail_m_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_trail_m_);
    glBufferData(GL_ARRAY_BUFFER,
                 trailCapacity_ * sizeof(glm::vec3),
                 nullptr,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), (void*)0);

    glGenVertexArrays(1, &vao_trail_t_);
    glGenBuffers(1, &vbo_trail_t_);
    glBindVertexArray(vao_trail_t_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_trail_t_);
    glBufferData(GL_ARRAY_BUFFER,
                 trailCapacity_ * sizeof(glm::vec3),
                 nullptr,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace vis
