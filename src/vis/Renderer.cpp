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

    // Set an initial global "map view" camera pose.
    setInitialGlobeView_();

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

    // Update view mode and handle smooth transitions (V key).
    double now = glfwGetTime();
    updateViewMode_(now, missileWorld);

    // Follow camera only when we're not in the middle of a view transition.
    if (!viewTrans_.active) {
        camController_.updateFollowToggle(window_);
        camController_.setFollowTarget(missileWorld);
    }

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

    // --------------------------------------------------------------------
    // Mode-dependent rendering: Globe vs Local.
    // For now they render the same geometry, but the separation lets us
    // change Local later (e.g., hide globe, show high-res terrain, etc.).
    // --------------------------------------------------------------------
    if (viewMode_ == ViewMode::Globe) {
        renderGlobeScene_(vp, missileWorld, targetWorld,
                          trail_m_globe, trail_t_globe);
    } else {
        renderLocalScene_(vp, missileWorld, targetWorld,
                          trail_m_globe, trail_t_globe);
    }
}

void Renderer::renderGlobeScene_(const glm::mat4& vp,
                                 const glm::vec3& missileWorld,
                                 const glm::vec3& targetWorld,
                                 const std::vector<glm::vec3>& trailMissile,
                                 const std::vector<glm::vec3>& trailTarget)
{
    // --------------------------------------------------------
    // Entity-based rendering: delegate to WorldScene.
    // --------------------------------------------------------
    if (master_) {
        world_.updateMissile(missileWorld);
        world_.submitGlobe(*master_, cam_);
    }

    // --------------------------------------------------------
    // Line renderer for trails / markers (and optionally axes).
    // --------------------------------------------------------
    lineRenderer_.begin(vp);

    // Trails
    lineRenderer_.drawTrail(
        lineRenderer_.missileTrailVAO(),
        lineRenderer_.missileTrailVBO(),
        trailMissile,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    lineRenderer_.drawTrail(
        lineRenderer_.targetTrailVAO(),
        lineRenderer_.targetTrailVBO(),
        trailTarget,
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

void Renderer::renderLocalScene_(const glm::mat4& vp,
                                 const glm::vec3& missileWorld,
                                 const glm::vec3& targetWorld,
                                 const std::vector<glm::vec3>& trailMissile,
                                 const std::vector<glm::vec3>& trailTarget)
{
    // --------------------------------------------------------
    // Entity-based rendering: local terrain + missile.
    // --------------------------------------------------------
    if (master_) {
        world_.updateMissile(missileWorld);
        world_.submitLocal(*master_, cam_);
    }

    // For now, reuse the same globe-space trails & markers.
    // Later we can switch these to ENU/local-only representations.
    lineRenderer_.begin(vp);

    lineRenderer_.drawTrail(
        lineRenderer_.missileTrailVAO(),
        lineRenderer_.missileTrailVBO(),
        trailMissile,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    lineRenderer_.drawTrail(
        lineRenderer_.targetTrailVAO(),
        lineRenderer_.targetTrailVBO(),
        trailTarget,
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    lineRenderer_.drawCross(missileWorld, 1.f, glm::vec3(0.0f, 1.0f, 0.0f));
    lineRenderer_.drawCross(targetWorld, 1.f, glm::vec3(1.0f, 0.0f, 0.0f));

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

void Renderer::startViewTransition_(ViewMode toMode,
                                    const glm::vec3& missileWorld)
{
    if (!window_) return;

    viewTrans_.active = true;
    viewTrans_.from   = viewMode_;
    viewTrans_.to     = toMode;
    viewTrans_.t      = 0.0f;

    // How long the transition takes (seconds).
    viewTrans_.duration = 2.0f; // tweak to taste

    // Start from current camera state.
    viewTrans_.posStart    = cam_.position();
    viewTrans_.targetStart = cam_.target();

    if (toMode == ViewMode::Local) {
        // -----------------------------
        // Local view: above the engagement, looking down.
        // -----------------------------
        glm::vec3 center(0.0f);

        // Radial direction from Earth center through the missile.
        glm::vec3 radial = glm::normalize(missileWorld - center);
        if (!std::isfinite(radial.x) ||
            !std::isfinite(radial.y) ||
            !std::isfinite(radial.z))
        {
            radial = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        // Radius of the missile point; fall back to mapper's radius if needed.
        float rMiss = glm::length(missileWorld);
        if (rMiss <= 0.0f) {
            rMiss = geoMapper_.earthWorldRadius();
            if (rMiss <= 0.0f) {
                rMiss = kEarthWorldRadius;
            }
        }

        // How far above the missile we want to be (in world units).
        const float backDist = 0.001f * rMiss;   // 50% of radius above surface (tune)

        // A small lateral offset so the camera isn't exactly nadir.
        const glm::vec3 worldUp(0.0f, 0.0f, 1.0f);
        glm::vec3 right = glm::normalize(glm::cross(radial, worldUp));
        if (glm::dot(right, right) < 1e-6f) {
            right = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        const float sideOffset = 0.1f * backDist;

        // Eye is above the missile along the radial, slightly off to the side.
        glm::vec3 eye = radial * (rMiss + backDist) + right * sideOffset;

        viewTrans_.posEnd    = eye;
        viewTrans_.targetEnd = missileWorld;  // look down toward the engagement
    } else {
        // -----------------------------
        // Globe view: far away, looking at Earth center.
        // -----------------------------
        glm::vec3 center(0.0f);

        glm::vec3 dir = glm::normalize(center - missileWorld);
        if (!std::isfinite(dir.x) ||
            !std::isfinite(dir.y) ||
            !std::isfinite(dir.z))
        {
            dir = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        float R = geoMapper_.earthWorldRadius();
        if (R <= 0.0f) {
            R = glm::length(missileWorld);
            if (R <= 0.0f) {
                R = kEarthWorldRadius;
            }
        }

        const float farDist = 3.0f * R;

        glm::vec3 camPos = center - dir * farDist;

        viewTrans_.posEnd    = camPos;
        viewTrans_.targetEnd = center;
    }
}

static glm::vec3 lerp(const glm::vec3& a,
                      const glm::vec3& b,
                      float t)
{
    return a + t * (b - a);
}

void Renderer::updateViewMode_(double now,
                               const glm::vec3& missileWorld)
{
    if (!window_) return;

    // Compute dt using a local time base.
    if (lastViewTime_ == 0.0) {
        lastViewTime_ = now;
    }
    float dt = static_cast<float>(now - lastViewTime_);
    lastViewTime_ = now;

    // Key toggle: 'V' switches Globe <-> Local.
    const int cur   = glfwGetKey(window_, GLFW_KEY_V);
    const bool down = (cur == GLFW_PRESS);
    if (down && !vPrevDown_ && !viewTrans_.active) {
        // Start a new transition.
        ViewMode targetMode = (viewMode_ == ViewMode::Globe)
                            ? ViewMode::Local
                            : ViewMode::Globe;
        startViewTransition_(targetMode, missileWorld);
    }
    vPrevDown_ = down;

    // If no transition is active, nothing more to do here.
    if (!viewTrans_.active) {
        return;
    }

    // Advance the transition timer.
    viewTrans_.t += dt;
    float alpha = viewTrans_.duration > 0.0f
                ? glm::clamp(viewTrans_.t / viewTrans_.duration, 0.0f, 1.0f)
                : 1.0f;

    // Smoothstep easing (nicer than linear).
    float s = alpha * alpha * (3.0f - 2.0f * alpha);

    glm::vec3 pos    = lerp(viewTrans_.posStart,    viewTrans_.posEnd,    s);
    glm::vec3 target = lerp(viewTrans_.targetStart, viewTrans_.targetEnd, s);

    cam_.setPosition(pos);
    cam_.setTarget(target);

    if (alpha >= 1.0f) {
        // Transition complete.
        viewMode_         = viewTrans_.to;
        viewTrans_.active = false;
    }
}

void Renderer::setInitialGlobeView_()
{
    // 1) Approximate the "missile world position" that the Globe transition
    //    code uses as its reference. At t=0 we don't have telemetry yet,
    //    so use the launch site / ENU origin on the globe.
    glm::vec3 missileWorld(0.0f);

    if (world_.hasLaunchMarker()) {
        // Launch marker is already on the globe surface; good proxy.
        missileWorld = world_.launchMarkerPos();
    } else {
        // Fallback: ENU(0,0,0) mapped to the globe via GeoMapper.
        missileWorld = geoMapper_.enuToGlobe(glm::vec3(0.0f));
    }

    // 2) Use the SAME camera formula as the Globe branch of startViewTransition_.
    glm::vec3 center(0.0f);

    glm::vec3 dir = glm::normalize(center - missileWorld);
    if (!std::isfinite(dir.x) ||
        !std::isfinite(dir.y) ||
        !std::isfinite(dir.z))
    {
        dir = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // Earth radius in world units.
    float R = geoMapper_.earthWorldRadius();
    if (R <= 0.0f) {
        R = glm::length(missileWorld);
        if (R <= 0.0f) {
            R = kEarthWorldRadius; // last-resort fallback
        }
    }

    const float farDist = 3.0f * R;

    glm::vec3 camPos = center - dir * farDist;

    // 3) Match the Globe transition: look at the Earth center.
    cam_.setTarget(center);
    cam_.setPosition(camPos);

    // 4) Ensure state is consistent with "already in Globe view".
    viewMode_          = ViewMode::Globe;
    viewTrans_.active  = false;
    viewTrans_.t       = 0.0f;
    lastViewTime_      = 0.0;
}


} // namespace vis
