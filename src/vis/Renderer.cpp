// Renderer.cpp
#include "vis/Renderer.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "vis/render/MasterRenderer.hpp"
#include "vis/models/RawModel.hpp"
#include "vis/models/ModelTexture.hpp"

#include "geo/GeoTypes.hpp"
#include "geo/GeoUtils.hpp"

namespace {

// GLFW error callback for easy debugging.
void glfw_error_callback(int code, const char* desc) {
    std::fprintf(stderr, "[GLFW] Error %d: %s\n", code, desc);
}

// Keep viewport in sync with framebuffer size.
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

static constexpr float kEarthWorldRadius = 10000.0f;

} // anonymous namespace

namespace vis {

bool Renderer::init(const RendererConfig& cfg)
{
    if (initialized_) {
        // Already initialized; nothing to do.
        return true;
    }

    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::fprintf(stderr, "[Viewer] glfwInit failed\n");
        return false;
    }

    // Request core profile 3.3.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(cfg.width, cfg.height, cfg.title, nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "[Viewer] glfwCreateWindow failed\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    // Load GL function pointers via GLAD.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "[Viewer] gladLoadGLLoader failed\n");
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    // VSync.
    glfwSwapInterval(cfg.vsync ? 1 : 0);

    // Initial viewport.
    glfwGetFramebufferSize(window_, &fbw_, &fbh_);
    glViewport(0, 0, fbw_, fbh_);
    glEnable(GL_DEPTH_TEST);

    // Camera setup.
    cam_.setViewport(fbw_, fbh_);
    cam_.setProj(60.f, 0.1f, 100000.f);

    // Attach camera to controller.
    camController_.attachCamera(&cam_);

    // Register GLFW callbacks (input, resize, etc.).
    setupCallbacks_();

    // Line renderer: shader + grid/axes + trail VBOs.
    lineRenderer_.init(
        "../res/shaders/line.vert",
        "../res/shaders/line.frag",
        trails_.capacity()
    );

    // Print GPU/GL info.
    std::fprintf(stderr, "[Viewer] GL Vendor  : %s\n", glGetString(GL_VENDOR));
    std::fprintf(stderr, "[Viewer] GL Renderer: %s\n", glGetString(GL_RENDERER));
    std::fprintf(stderr, "[Viewer] GL Version : %s\n", glGetString(GL_VERSION));

    // Create the master renderer using the current camera projection.
    master_ = std::make_unique<MasterRenderer>(
        cam_,
        "../res/shaders/entity.vert",
        "../res/shaders/entity.frag"
    );
    master_->setSkyColor(glm::vec3(0.35f, 0.55f, 0.9f));

    // World scene: Earth, terrain, missile, ground, sun, launch marker.
    world_.init(loader_, geoMapper_, origin_);

    initialized_ = true;
    return true;
}

bool Renderer::beginFrame()
{
    if (!initialized_ || !window_) {
        return false;
    }

    glfwPollEvents();
    glfwGetFramebufferSize(window_, &fbw_, &fbh_);
    glViewport(0, 0, fbw_, fbh_);

    return true;
}

void Renderer::drawScene()
{
    if (!initialized_ || !window_) {
        return;
    }

    // Recompute projection on resize.
    cam_.setViewport(fbw_, fbh_);
    cam_.setProj(60.f, 0.1f, 100000.f);

    // Update telemetry first so trails_ has fresh ENU positions.
    drainBus_();

    // Use last samples from the TrailSystem (defaults to {0,0,0} if none yet).
    const glm::vec3 missileEnu = trails_.lastMissile();
    const glm::vec3 targetEnu  = trails_.lastTarget();

    // Compute globe-space positions for missile/target.
    const glm::vec3 missileWorld = geoMapper_.enuToGlobe(missileEnu);
    const glm::vec3 targetWorld  = geoMapper_.enuToGlobe(targetEnu);

    // Follow toggle (uses current missile position on the globe).
    camController_.updateFollowToggle(window_);
    camController_.setFollowTarget(missileWorld);

    // If you later implement ensureOutsideSphere, use geoMapper_ here.
    // if (geoMapper_.isReady()) {
    //     cam_.ensureOutsideSphere(glm::vec3(0.0f),
    //                              geoMapper_.earthRadius(),
    //                              100.0f);
    // }

    // Now that camera may have changed, get view/proj/vp.
    const glm::mat4 view = cam_.view();
    const glm::mat4 proj = cam_.proj();
    const glm::mat4 vp   = proj * view;

    // Build globe-space trails from ENU trails.
    static std::vector<glm::vec3> trail_m_globe;
    static std::vector<glm::vec3> trail_t_globe;
    trail_m_globe.clear();
    trail_t_globe.clear();

    const auto& missileTrailEnu = trails_.missileTrail();
    const auto& targetTrailEnu  = trails_.targetTrail();

    trail_m_globe.reserve(missileTrailEnu.size());
    trail_t_globe.reserve(targetTrailEnu.size());

    for (const auto& p : missileTrailEnu) {
        trail_m_globe.push_back(geoMapper_.enuToGlobe(p));
    }
    for (const auto& p : targetTrailEnu) {
        trail_t_globe.push_back(geoMapper_.enuToGlobe(p));
    }

    // --------------------------------------------------------
    // Entity-based rendering: delegate to WorldScene.
    // --------------------------------------------------------
    if (master_) {
        world_.updateMissile(missileWorld);
        world_.submit(*master_, cam_);
    }

    // --------------------------------------------------------
    // Line renderer for trails / markers (and optionally axes).
    // --------------------------------------------------------
    lineRenderer_.begin(vp);

    // Trails
    lineRenderer_.drawTrail(
        lineRenderer_.missileTrailVAO(),
        lineRenderer_.missileTrailVBO(),
        trail_m_globe,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    lineRenderer_.drawTrail(
        lineRenderer_.targetTrailVAO(),
        lineRenderer_.targetTrailVBO(),
        trail_t_globe,
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    // Optional: axes
    // lineRenderer_.drawAxes();

    // Missile marker on globe
    lineRenderer_.drawCross(missileWorld, 1.f, glm::vec3(0.0f, 1.0f, 0.0f));

    // Target marker on globe
    lineRenderer_.drawCross(targetWorld, 1.f, glm::vec3(1.0f, 0.0f, 0.0f));

    // Launch site marker on globe.
    if (world_.hasLaunchMarker()) {
        lineRenderer_.drawCross(
            world_.launchMarkerPos(),
            3.0f,
            glm::vec3(0.0f, 0.0f, 0.0f)
        );
    }

    lineRenderer_.end();
}

void Renderer::endFrame()
{
    if (!initialized_ || !window_) {
        return;
    }
    glfwSwapBuffers(window_);
}

bool Renderer::shouldClose() const
{
    return !window_ || glfwWindowShouldClose(window_);
}

void Renderer::shutdown()
{
    // Idempotent guard.
    if (!initialized_ && !window_) {
        return;
    }

    initialized_ = false;

    // Stop consuming external data during teardown.
    bus_ = nullptr;

    // Ensure THIS context is current before any GL destruction.
    if (window_) {
        glfwMakeContextCurrent(window_);
    }

    // --- Delete GL resources while context is alive. ---
    lineRenderer_.shutdown();

    // Entity renderer + shaders should be destroyed while context is alive.
    master_.reset();

    // Loader cleanup (textures, VAOs, etc.).
    loader_.cleanUp();

    // Destroy the window last (kills the context).
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    // Since this app only uses GLFW for this viewer, match glfwInit() with
    // glfwTerminate() here.
    glfwTerminate();

    fbw_ = 0;
    fbh_ = 0;
}

void Renderer::drainBus_()
{
    if (!bus_) return;

    static std::vector<TelemetrySample> tmp;
    tmp.clear();

    const std::size_t n = bus_->drain(tmp, 16384);
    if (n == 0) {
        return;
    }

    for (const auto& s : tmp) {
        const glm::vec3 pm{s.mx, s.my, s.mz};
        const glm::vec3 pt{s.tx, s.ty, s.tz};
        trails_.addSample(pm, pt);
    }
}

void Renderer::setGeoOrigin(const geo::GeoOrigin* origin)
{
    origin_ = origin;
    geoMapper_.setOrigin(origin);
    // Optional future: if (initialized_) re-init world_ or terrain patch.
}
void Renderer::setupCallbacks_()
{
    if (!window_) return;

    // Associate this Renderer instance with the GLFW window.
    glfwSetWindowUserPointer(window_, this);

    // Keep viewport in sync with framebuffer size.
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

    // Mouse move → CameraController
    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double x, double y) {
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if (!self) return;
        self->camController_.onMouseMove(x, y);
    });

    // Mouse buttons → CameraController
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        double x, y;
        glfwGetCursorPos(w, &x, &y);
        self->camController_.onMouseButton(button, action, mods, x, y);
    });

    // Scroll wheel → CameraController
    glfwSetScrollCallback(window_, [](GLFWwindow* w, double /*xoff*/, double yoff) {
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if (!self) return;
        self->camController_.onScroll(yoff);
    });
}



} // namespace vis
