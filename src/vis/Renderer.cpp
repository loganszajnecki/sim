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

    // Callbacks.
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

    // Initial viewport.
    glfwGetFramebufferSize(window_, &fbw_, &fbh_);
    glViewport(0, 0, fbw_, fbh_);

    // Basic GL state.
    glEnable(GL_DEPTH_TEST);

    // Compile line shader.
    lineShader_ = std::make_unique<LineShader>(
        "../res/shaders/line.vert",
        "../res/shaders/line.frag"
    );

    // Camera setup.
    cam_.setViewport(fbw_, fbh_);
    cam_.setProj(60.f, 0.1f, 100000.f);

    // Static geometry (grid + axes).
    initGridAxes_();

    // Dynamic VBOs for trails.
    initDynamicVBOs_();

    // Basic mouse input.
    glfwSetWindowUserPointer(window_, this);

    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double x, double y) {
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        const float dx = static_cast<float>(x - self->lastx_);
        const float dy = static_cast<float>(y - self->lasty_);

        if (self->orbiting_) {
            self->cam_.orbit(dx, dy); // pixels
        } else if (self->panning_) {
            self->cam_.pan(dx, dy);   // pixels
        }

        self->lastx_ = x;
        self->lasty_ = y;
    });

    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                self->orbiting_ = !(mods & GLFW_MOD_SHIFT); // LMB: orbit
                self->panning_  =  (mods & GLFW_MOD_SHIFT); // Shift+LMB: pan
                // Reset deltas on press to avoid jump.
                double x, y;
                glfwGetCursorPos(w, &x, &y);
                self->lastx_ = x;
                self->lasty_ = y;
            } else if (action == GLFW_RELEASE) {
                self->orbiting_ = false;
                self->panning_  = false;
            }
        }

        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                self->panning_ = true;
                double x, y;
                glfwGetCursorPos(w, &x, &y);
                self->lastx_ = x;
                self->lasty_ = y;
            } else if (action == GLFW_RELEASE) {
                self->panning_ = false;
            }
        }
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* w, double /*xoff*/, double yoff) {
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if (!self) return;
        self->cam_.dolly(static_cast<float>(yoff)); // positive yoff zooms in
    });

    // Print GPU/GL info.
    std::fprintf(stderr, "[Viewer] GL Vendor  : %s\n", glGetString(GL_VENDOR));
    std::fprintf(stderr, "[Viewer] GL Renderer: %s\n", glGetString(GL_RENDERER));
    std::fprintf(stderr, "[Viewer] GL Version : %s\n", glGetString(GL_VERSION));

    // --------------------------------------------------------
    // Entity-based rendering setup (MasterRenderer + missile).
    // --------------------------------------------------------
    // Simple far-away sun light.
    sun_.position = glm::vec3(kEarthWorldRadius*3.0f, kEarthWorldRadius*3.0f, kEarthWorldRadius*3.0f);
    sun_.color    = glm::vec3(1.0f, 1.0f, 1.0f);

    // Create the master renderer using the current camera projection.
    master_ = std::make_unique<MasterRenderer>(
        cam_,
        "../res/shaders/entity.vert",
        "../res/shaders/entity.frag"
    );
    master_->setSkyColor(glm::vec3(0.35f, 0.55f, 0.9f));

    try {
        RawModel missileRaw = OBJLoader::loadObjModel("tree", loader_);

        ModelTexture missileTex{};
        missileTex.id             = loader_.loadTexture("tree");
        missileTex.shineDamper    = 10.0f;
        missileTex.reflectivity   = 0.9f;
        missileTex.hasTransparency= false;
        missileTex.useFakeLighting= false;

        missileModel_  = TexturedModel{missileRaw, missileTex};
        missileEntity_ = Entity(&missileModel_,
                                glm::vec3(0.0f),
                                glm::vec3{90.0f, 0.0f, 0.0f},
                                20.0f);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Renderer] Failed to load missile OBJ: %s\n", e.what());
        missileEntity_.model    = nullptr;
        missileEntity_.position = glm::vec3(0.0f);
        missileEntity_.rotation = glm::vec3(0.0f);
        missileEntity_.scale    = 1.0f;
    }

    // --------------------------------------------------------
    // Earth sphere + launch marker (Phase 2).
    // --------------------------------------------------------
    try {
        // Use your Blender-exported earth.obj + 8k_earth_daymap texture
        RawModel earthRaw = OBJLoader::loadObjModel("earth", loader_);

        // Query the true model-space radius just computed.
        float earthModelRadius = OBJLoader::getModelRadius("earth");

        // Compute scale so that model radius -> desired world radius.
        float earthScale = kEarthWorldRadius / earthModelRadius;
        earthWorldRadius_ = kEarthWorldRadius;

        ModelTexture earthTex{};
        earthTex.id              = loader_.loadTexture("8k_earth_daymap");
        earthTex.shineDamper     = 10.0f;
        earthTex.reflectivity    = 0.0f;
        earthTex.hasTransparency = false;
        earthTex.useFakeLighting = false;

        earthModel_  = TexturedModel{earthRaw, earthTex};
        earthEntity_ = Entity(&earthModel_,
                      glm::vec3(0.0f),
                      glm::vec3(0.0f),
                      earthScale);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Renderer] Failed to load Earth OBJ/texture: %s\n", e.what());
        earthEntity_.model    = nullptr;
        earthEntity_.position = glm::vec3(0.0f);
        earthEntity_.rotation = glm::vec3(0.0f);
        earthEntity_.scale    = 1.0f;
    }

    // Init launch marker entity (position later).
    launchMarkerEntity_.model    = nullptr;
    launchMarkerEntity_.position = glm::vec3(0.0f);
    launchMarkerEntity_.rotation = glm::vec3(0.0f);
    launchMarkerEntity_.scale    = 1.0f;

    // Compute launch marker position if we know the origin and Earth loaded.
    if (origin_ && earthEntity_.model) {
        // Direction from Earth center to origin in ECEF
        glm::dvec3 ecef = geo::ecefFromLLA(origin_->lla);
        glm::dvec3 dir  = glm::normalize(ecef);

        // Visual position on sphere
        glm::vec3 markerPos = glm::vec3(dir) * (kEarthWorldRadius * 1.01f);

        launchMarkerEntity_.position = markerPos;
        launchMarkerEntity_.rotation = glm::vec3(0.0f);  // no extra rotation needed
        launchMarkerEntity_.scale    = 300.0f;

        launchMarkerPos_      = markerPos;
        haveLaunchMarkerPos_  = true;
    }

    // Ground is optional; keep as null unless explicitly loaded.
    groundEntity_.model    = nullptr;
    groundEntity_.position = glm::vec3(0.0f);
    groundEntity_.rotation = glm::vec3(0.0f);
    groundEntity_.scale    = 1.0f;

    // --------------------------------------------------------
    // Local terrain patch around the geo origin
    // --------------------------------------------------------
    initTerrainPatch_();

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

    // Update telemetry first so last_m_ / last_t_ are fresh ENU positions.
    drainBus_();

    // Compute globe-space positions for missile/target.
    const glm::vec3 missileWorld = enuToGlobeVisual_(last_m_);
    const glm::vec3 targetWorld  = enuToGlobeVisual_(last_t_);

    // Follow toggle (uses current missile position on the globe).
    updateFollowToggle_();
    if (followEnabled_) {
        cam_.setTarget(missileWorld); // follow missile on the globe
    }

    // Keep the camera outside the Earth sphere.
    if (earthEnabled_ && earthEntity_.model) {
        // cam_.ensureOutsideSphere(glm::vec3(0.0f), earthWorldRadius_, 100.0f);
    }

    // Now that camera may have changed, get view/proj/vp.
    const glm::mat4 view = cam_.view();
    const glm::mat4 proj = cam_.proj();
    const glm::mat4 vp   = proj * view;

    static std::vector<glm::vec3> trail_m_globe;
    static std::vector<glm::vec3> trail_t_globe;
    trail_m_globe.clear();
    trail_t_globe.clear();
    trail_m_globe.reserve(trail_m_.size());
    trail_t_globe.reserve(trail_t_.size());
    for (const auto& p : trail_m_) {
        trail_m_globe.push_back(enuToGlobeVisual_(p));
    }
    for (const auto& p : trail_t_) {
        trail_t_globe.push_back(enuToGlobeVisual_(p));
    }
    // --------------------------------------------------------
    // Entity-based rendering: Earth, launch marker, missile, ground.
    // --------------------------------------------------------
    if (master_) {
        if (earthEnabled_ && earthEntity_.model) {
            master_->processEntity(earthEntity_);
        }

        if (terrainEnabled_ && terrainEntity_.model) {
            master_->processEntity(terrainEntity_);
        }

        if (earthEnabled_ && launchMarkerEntity_.model) {
            master_->processEntity(launchMarkerEntity_);
        }

        if (missileEntity_.model) {
            missileEntity_.position = missileWorld;
            master_->processEntity(missileEntity_);
        }

        if (groundEntity_.model) {
            master_->processEntity(groundEntity_);
        }

        master_->render(sun_, cam_);
    }

    // --------------------------------------------------------
    // Line shader for axes / trails / markers.
    // --------------------------------------------------------
    lineShader_->start();
    lineShader_->loadVP(vp);

    drawTrail_(vao_trail_m_, vbo_trail_m_, trail_m_globe, {0.0f, 1.0f, 0.0f});
    drawTrail_(vao_trail_t_, vbo_trail_t_, trail_t_globe, {1.0f, 0.0f, 0.0f});


    // Optional: missile marker on globe
    drawMarkerCross_(missileWorld, 20.f, {0.0f, 1.0f, 0.0f});

    // Target marker on globe (only once).
    drawMarkerCross_(targetWorld, 60.f, {1.0f, 0.0f, 0.0f});

    // Launch site marker on globe.
    if (earthEnabled_ && haveLaunchMarkerPos_) {
        drawMarkerCross_(launchMarkerPos_, 500.0f,
                         glm::vec3(1.0f, 1.0f, 0.0f));
    }

    lineShader_->stop();
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
    if (vao_grid_)  { glDeleteVertexArrays(1, &vao_grid_);  vao_grid_  = 0; }
    if (vbo_grid_)  { glDeleteBuffers(1,       &vbo_grid_);  vbo_grid_  = 0; }
    count_grid_ = 0;

    if (vao_axes_)  { glDeleteVertexArrays(1, &vao_axes_);  vao_axes_  = 0; }
    if (vbo_axes_)  { glDeleteBuffers(1,       &vbo_axes_);  vbo_axes_  = 0; }
    count_axes_ = 0;

    if (vao_trail_m_) { glDeleteVertexArrays(1, &vao_trail_m_); vao_trail_m_ = 0; }
    if (vbo_trail_m_) { glDeleteBuffers(1,       &vbo_trail_m_); vbo_trail_m_ = 0; }

    if (vao_trail_t_) { glDeleteVertexArrays(1, &vao_trail_t_); vao_trail_t_ = 0; }
    if (vbo_trail_t_) { glDeleteBuffers(1,       &vbo_trail_t_); vbo_trail_t_ = 0; }

    // Entity renderer + shaders should be destroyed while context is alive.
    master_.reset();
    lineShader_.reset();

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

// Z-up: grid on the XY plane (Z = 0), axes: X=red, Y=green, Z=blue.
void Renderer::initGridAxes_()
{
    // Grid (2 km x 2 km, 10 m spacing).
    const int   half = 1000;   // 100 * 10 m each side => 2 km span.
    const float step = 500.f;  // 10 meters.
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

void Renderer::initDynamicVBOs_()
{
    glGenVertexArrays(1, &vao_trail_m_);
    glGenBuffers(1, &vbo_trail_m_);
    glBindVertexArray(vao_trail_m_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_trail_m_);
    glBufferData(GL_ARRAY_BUFFER,
                 trail_cap_ * sizeof(glm::vec3),
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
                 trail_cap_ * sizeof(glm::vec3),
                 nullptr,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
        trail_m_.push_back(pm);
        trail_t_.push_back(pt);
        last_m_ = pm;
        last_t_ = pt;
    }

    // Cap trail lengths.
    auto clip = [&](std::vector<glm::vec3>& v) {
        if (v.size() > trail_cap_) {
            v.erase(v.begin(), v.begin() + (v.size() - trail_cap_));
        }
    };
    clip(trail_m_);
    clip(trail_t_);
}

void Renderer::drawTrail_(GLuint vao,
                          GLuint vbo,
                          const std::vector<glm::vec3>& pts,
                          const glm::vec3& color)
{
    if (pts.empty()) return;

    lineShader_->loadColor(color);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    pts.size() * sizeof(glm::vec3),
                    pts.data());

    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(pts.size()));
    glBindVertexArray(0);
}

void Renderer::drawMarkerCross_(const glm::vec3& p,
                                float L,
                                const glm::vec3& color)
{
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

    lineShader_->loadColor(color);
    glDrawArrays(GL_LINES, 0, 6);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

glm::vec3 Renderer::enuToGlobeVisual_(const glm::vec3& enuLocal) const
{
    // If we don't have a geo origin or Earth model, just return ENU as-is.
    if (!origin_ || !earthEntity_.model) {
        return enuLocal;
    }
    // TODO: When local terrain patch is in place, revisit this mapping
    //       (use true altitude locally, and only use this for far/overview view).
    // NOTE: This is a visualization-only mapping:
    //  - We exaggerate horizontal ENU displacement to make motion visible on the globe.
    //  - We ignore altitude (enuLocal.z) and always place points on the visual Earth surface.
    constexpr double ENU_VISUAL_SCALE = 1.0; // tweak for debug visibility

    glm::dvec3 enuScaled = glm::dvec3(enuLocal) * ENU_VISUAL_SCALE;

    // ENU (local) -> ECEF (meters)
    glm::dvec3 ecef = origin_->ecef + origin_->enu_to_ecef * enuScaled;

    // Direction from Earth center.
    glm::vec3 dir = glm::normalize(glm::vec3(ecef));

    static constexpr float kObjectsOffset = 0.0f; // missile/cross above patch
    return dir * (earthWorldRadius_ + kObjectsOffset);
}


void Renderer::updateFollowToggle_() 
{
    const int cur   = glfwGetKey(window_, GLFW_KEY_F);
    const bool down = (cur == GLFW_PRESS);
    if (down && !fPrevDown_) {
        followEnabled_ = !followEnabled_;
    }
    fPrevDown_ = down;
}

void Renderer::initTerrainPatch_() 
{
    terrainEnabled_ = false;

    // Need a geo origin and an Earth model to build a patch
    if (!origin_ || !earthEntity_.model) {
        return;
    }

    // Patch settings: size and resolution.
    const float HALF_SIZE = 50000.0f; // meters in enu
    const int N = 64;

    const int VERTS_X = N + 1;
    const int VERTS_Y = N + 1;
    
    std::vector<float> positions;
    std::vector<float> texcoords;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

    positions.reserve(VERTS_X * VERTS_Y * 3);
    texcoords.reserve(VERTS_X * VERTS_Y * 2);
    normals.reserve(VERTS_X * VERTS_Y * 3);
    indices.reserve(N * N * 6);

    for (int j = 0; j < VERTS_Y; ++j) {
        float v     = static_cast<float>(j) / static_cast<float>(N);
        float yENU  = (v * 2.0f - 1.0f) * HALF_SIZE; // north

        for (int i = 0; i < VERTS_X; ++i) {
            float u     = static_cast<float>(i) / static_cast<float>(N);
            float xENU  = (u * 2.0f - 1.0f) * HALF_SIZE; // east

            // Flat patch in ENU at z=0 (we’ll add real height later).
            glm::dvec3 enu(xENU, yENU, 0.0);

            // ENU -> ECEF using origin basis.
            glm::dvec3 ecef = origin_->ecef + origin_->enu_to_ecef * enu;

            // Project onto visual Earth sphere.
            glm::vec3 dir = glm::normalize(glm::vec3(ecef));
            // tiny visual offset to avoid z-fighting
            static constexpr float kPatchOffset = 10.0f;
            glm::vec3 pos = dir * (earthWorldRadius_ + kPatchOffset);

            // Positions.
            positions.push_back(pos.x);
            positions.push_back(pos.y);
            positions.push_back(pos.z);

            // UVs: simple [0,1] over the patch.
            texcoords.push_back(u);
            texcoords.push_back(v);

            // Normals ≈ radial from Earth center (good enough for lighting).
            normals.push_back(dir.x);
            normals.push_back(dir.y);
            normals.push_back(dir.z);
        }
    }

    // Build indices for a regular grid.
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            int i0 = j * VERTS_X + i;
            int i1 = i0 + 1;
            int i2 = i0 + VERTS_X;
            int i3 = i2 + 1;

            // New: CCW as seen from *outside* the Earth (faces up/outward).
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    // Upload to GPU
    terrainRaw_ = loader_.loadToVAO(
        positions,
        texcoords,
        normals,
        indices
    );
    // reuse earth texture for now
    ModelTexture terrainTex{};
    terrainTex.id              = loader_.loadTexture("terrain_local"); // make sure this exists
    terrainTex.shineDamper     = 4.0f;
    terrainTex.reflectivity    = 0.0f;
    terrainTex.hasTransparency = false;
    terrainTex.useFakeLighting = false;

    terrainModel_  = TexturedModel{terrainRaw_, terrainTex};
    terrainEntity_ = Entity(&terrainModel_,
                            glm::vec3(0.0f),             // positions already in world
                            glm::vec3(0.0f, 0.0f, 0.0f),
                            1.0f);                       // no scaling

    terrainEnabled_ = true;


}

void Renderer::setGeoOrigin(const geo::GeoOrigin* origin)
{
    origin_ = origin;
}


} // namespace vis
