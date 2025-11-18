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
#include "vis/TrailSystem.hpp"
#include "vis/render/LineRenderer.hpp"
#include "vis/CameraController.hpp"
#include "vis/geo/GeoMapper.hpp"
#include "vis/geo/TerrainPatchBuilder.hpp"
#include "vis/render/MasterRenderer.hpp"
#include "vis/models/TexturedModel.hpp"
#include "vis/entities/Entity.hpp"
#include "vis/entities/Light.hpp"
#include "vis/Loader.hpp"
#include "vis/OBJLoader.hpp"
#include "vis/shaders/LineShader.hpp"
#include "vis/scene/WorldScene.hpp"
#include "geo/GeoTypes.hpp"

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
 * @brief View modes for camera state machine
 */
enum class ViewMode {
    Globe,
    Local
};

/**
 * @brief Parameters for transitioning from Globe<->Local
 */
struct ViewTransition {
    bool active           = false;
    ViewMode from         = ViewMode::Globe;
    ViewMode to           = ViewMode::Local;
    float t               = 0.0f;       // elapsed time (seconds)
    float duration        = 2.0f;       // total transition time (seconds)
    glm::vec3 posStart    = glm::vec3(0.0f);
    glm::vec3 posEnd      = glm::vec3(0.0f);
    glm::vec3 targetStart = glm::vec3(0.0f);
    glm::vec3 targetEnd   = glm::vec3(0.0f);
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

    // Tell the renderer what Earth origin to use
    void setGeoOrigin(const geo::GeoOrigin* origin);
private:
    void drainBus_();
    void setupCallbacks_();
    bool initialized_{false};

    GLFWwindow* window_{nullptr};
    int fbw_{0};
    int fbh_{0}; // framebuffer size (for viewport)

    // Camera + line renderer.
    Camera           cam_;
    CameraController camController_;
    LineRenderer     lineRenderer_;

    // Static line geometry.
    GLuint vao_grid_{0}, vbo_grid_{0};
    GLuint vao_axes_{0}, vbo_axes_{0};
    GLuint count_grid_{0}, count_axes_{0};

    // Dynamic trails.
    GLuint vao_trail_m_{0}, vbo_trail_m_{0};
    GLuint vao_trail_t_{0}, vbo_trail_t_{0};

    // CPU-side trail storage (RAII).
    TrailSystem trails_{5000};

    // Non-owning telemetry queue (owned by RendererController).
    TelemetryBus* bus_{nullptr};

    // ----------------------------------------------------------------
    // Entity-based rendering pieces.
    // ----------------------------------------------------------------
    std::unique_ptr<MasterRenderer> master_;

    // Geo mapping (ENU -> globe) and Earth/terrain entities.
    GeoMapper    geoMapper_;

    Loader    loader_;
    const geo::GeoOrigin* origin_ = nullptr; // ENU origin / launch site

    WorldScene world_;

    // --- View mode + smooth transition between global and local views ---
    ViewMode        viewMode_{ViewMode::Globe};
    ViewTransition  viewTrans_;
    bool vPrevDown_{false};    // edge detect for 'V' key
    double lastViewTime_{0.0}; // for dt in drawScene
    void updateViewMode_(double now,
                         const glm::vec3& missileWorld);
    void startViewTransition_(ViewMode toMode,
                              const glm::vec3& missileWorld);
    void setInitialGlobeView_();

    void renderGlobeScene_(const glm::mat4& vp,
                           const glm::vec3& missileWorld,
                           const glm::vec3& targetWorld,
                           const std::vector<glm::vec3>& trailMissile,
                           const std::vector<glm::vec3>& trailTarget);

    void renderLocalScene_(const glm::mat4& vp,
                           const glm::vec3& missileWorld,
                           const glm::vec3& targetWorld,
                           const std::vector<glm::vec3>& trailMissile,
                           const std::vector<glm::vec3>& trailTarget);
};

} // namespace vis
