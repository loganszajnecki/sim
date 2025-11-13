// Renderer.cpp
#include "vis/Renderer.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "vis/render/MasterRenderer.hpp"
#include "vis/models/RawModel.hpp"
#include "vis/models/ModelTexture.hpp"

namespace {

// GLFW error callback for easy debugging.
void glfw_error_callback(int code, const char* desc) {
    std::fprintf(stderr, "[GLFW] Error %d: %s\n", code, desc);
}

// Keep viewport in sync with framebuffer size.
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

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
    sun_.position = glm::vec3(20000.0f, 20000.0f, 20000.0f);
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

    // Ground is optional; keep as null unless explicitly loaded.
    groundEntity_.model    = nullptr;
    groundEntity_.position = glm::vec3(0.0f);
    groundEntity_.rotation = glm::vec3(0.0f);
    groundEntity_.scale    = 1.0f;

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

    const glm::mat4 view = cam_.view();
    const glm::mat4 proj = cam_.proj();
    const glm::mat4 vp   = proj * view;

    // Update telemetry first so last_m_ / last_t_ are fresh.
    drainBus_();

    updateFollowToggle_();

    // Choose camera target based on mode
    if (followEnabled_) {
        cam_.setTarget(last_m_);
    }

    // Entity-based rendering: missile, ground, etc.
    if (master_) {
        // Use last_m_ as missile world position.
        missileEntity_.position = last_m_;
        master_->processEntity(missileEntity_);
        master_->render(sun_, cam_);
    }

    // Line shader for grid / axes / trails / markers.
    lineShader_->start();
    lineShader_->loadVP(vp);

    // Grid.
    if (vao_grid_ != 0 && count_grid_ > 0) {
        lineShader_->loadColor(glm::vec3(0.35f, 0.37f, 0.40f));
        glBindVertexArray(vao_grid_);
        glDrawArrays(GL_LINES, 0, count_grid_);
    }

    // Axes (RGB).
    if (vao_axes_ != 0 && count_axes_ > 0) {
        glBindVertexArray(vao_axes_);
        // X (red).
        lineShader_->loadColor(glm::vec3(0.9f, 0.2f, 0.2f));
        glDrawArrays(GL_LINES, 0, 2);
        // Y (green).
        lineShader_->loadColor(glm::vec3(0.2f, 0.9f, 0.2f));
        glDrawArrays(GL_LINES, 2, 2);
        // Z (blue).
        lineShader_->loadColor(glm::vec3(0.2f, 0.4f, 0.9f));
        glDrawArrays(GL_LINES, 4, 2);
    }
    glBindVertexArray(0);

    // Trails + markers.
    drawTrail_(vao_trail_m_, vbo_trail_m_, trail_m_, {0.0f, 1.0f, 0.0f});
    drawTrail_(vao_trail_t_, vbo_trail_t_, trail_t_, {1.0f, 0.0f, 0.0f});

    //drawMarkerCross_(last_m_, 20.f, {0.0f, 1.0f, 0.0f});
    drawMarkerCross_(last_t_, 60.f, {1.0f, 0.0f, 0.0f});

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

void Renderer::updateFollowToggle_() 
{
    const int cur   = glfwGetKey(window_, GLFW_KEY_F);
    const bool down = (cur == GLFW_PRESS);
    if (down && !fPrevDown_) {
        followEnabled_ = !followEnabled_;
    }
    fPrevDown_ = down;
}

} // namespace vis
