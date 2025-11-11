// Renderer.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>

#include <glad/glad.h>
struct GLFWwindow;

#include <glm/glm.hpp>

#include "vis/entities/Camera.hpp"
#include "vis/Telemetry.hpp"

#include "vis/render/MasterRenderer.hpp"
#include "vis/models/TexturedModel.hpp"
#include "vis/entities/Entity.hpp"
#include "vis/entities/Light.hpp"
#include "vis/Loader.hpp"
#include "vis/OBJLoader.hpp"
#include "vis/shaders/LineShader.hpp"

namespace vis {

/**
 * @brief Window + OpenGL viewer configuration.
 */
struct RendererConfig {
    int  width  = 1920;
    int  height = 1080;
    bool vsync  = true;
    const char* title = "SAM Viewer";
};

/**
 * @brief Viewer + rendering engine for the SAM visualizer.
 *
 * Responsibilities:
 *  - Owns the GLFW window and OpenGL context.
 *  - Owns GL resources (VAOs/VBOs) for grid/axes/trails.
 *  - Owns an entity-style renderer (MasterRenderer + models/textures).
 *  - Consumes telemetry samples via a TelemetryBus pointer (non-owning).
 *
 * RAII:
 *  - init() acquires GL resources and creates the window/context.
 *  - shutdown() (also called from ~Renderer) destroys GL resources,
 *    the window, and terminates GLFW. It is safe and idempotent.
 */
class Renderer
{
public:
    Renderer() = default;
    ~Renderer() noexcept { shutdown(); }

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&)                 = delete;
    Renderer& operator=(Renderer&&)      = delete;

    /**
     * @brief Initialize the viewer and renderer.
     *
     * Safe to call multiple times; after a successful init, further calls
     * will simply return true.
     */
    [[nodiscard]] bool init(const RendererConfig& cfg = {});

    /// Poll events, update viewport, and clear the framebuffer.
    [[nodiscard]] bool beginFrame();

    /// Main draw routine: entities, grid, axes, trails, markers.
    void drawScene();

    /// Swap front/back buffers.
    void endFrame();

    /// Shutdown and release all resources (safe, idempotent).
    void shutdown();

    /// @return true if the window should close (or if no window exists).
    [[nodiscard]] bool shouldClose() const;

    /// @return true if init() has completed successfully.
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }

    /// Attach a non-owning telemetry bus pointer (must outlive this renderer).
    void attachBus(TelemetryBus* bus) { bus_ = bus; }

private:
    void initGridAxes_();
    void initDynamicVBOs_();
    void drainBus_();
    void drawTrail_(GLuint vao,
                    GLuint vbo,
                    const std::vector<glm::vec3>& pts,
                    const glm::vec3& color);
    void drawMarkerCross_(const glm::vec3& p,
                          float L,
                          const glm::vec3& color);

    bool initialized_{false};

    GLFWwindow* window_{nullptr};
    int fbw_{0};
    int fbh_{0}; // framebuffer size (for viewport)

    // Camera and line shader.
    Camera                         cam_;
    std::unique_ptr<LineShader>    lineShader_;

    // Static line geometry.
    GLuint vao_grid_{0}, vbo_grid_{0};
    GLuint vao_axes_{0}, vbo_axes_{0};
    GLuint count_grid_{0}, count_axes_{0};

    // Dynamic trails.
    GLuint vao_trail_m_{0}, vbo_trail_m_{0};
    GLuint vao_trail_t_{0}, vbo_trail_t_{0};
    std::size_t trail_cap_{5000};

    std::vector<glm::vec3> trail_m_;
    std::vector<glm::vec3> trail_t_;
    glm::vec3              last_m_{0.f};
    glm::vec3              last_t_{0.f};

    // Non-owning telemetry queue (owned by RendererController).
    TelemetryBus* bus_{nullptr};

    // Input state.
    bool   orbiting_{false}; // LMB drag
    bool   panning_{false};  // MMB drag or Shift+LMB
    double lastx_{0.0};
    double lasty_{0.0};

    // ----------------------------------------------------------------
    // Entity-based rendering pieces.
    // ----------------------------------------------------------------
    std::unique_ptr<MasterRenderer> master_;

    TexturedModel groundModel_;
    Entity        groundEntity_;

    TexturedModel missileModel_;
    Entity        missileEntity_;

    Light  sun_;
    Loader loader_;
};

} // namespace vis
