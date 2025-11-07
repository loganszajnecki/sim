#include "vis/Renderer.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace {

void glfw_error_callback(int code, const char* desc) {
    std::fprintf(stderr, "[GLFW] Error %d: %s\n", code, desc);
}

void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

const char* VS_LINES = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uVP;
void main(){ gl_Position = uVP * vec4(aPos,1.0); }
)";

const char* FS_LINES = R"(#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main(){ FragColor = vec4(uColor,1.0); }
)";

const char* VS_MESH_LIT = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Assume no non-uniform scale for normals
    vNormal = mat3(uModel) * aNormal;

    gl_Position = uProj * uView * worldPos;
}
)";


const char* FS_MESH_LIT = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3  uLightPos;       // point light position (world space)
uniform vec3  uBaseColor;      // base albedo
uniform float uLightIntensity; // <--- NEW

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);

    float NdotL = max(dot(N, L), 0.0);

    float ambient = 0.30;
    float diffuse = NdotL;

    // scale by intensity
    float lighting = (ambient + diffuse) * uLightIntensity;

    FragColor = vec4(uBaseColor * lighting, 1.0);
}
)";



} // namespace

namespace vis {

bool Renderer::init(const RendererConfig& cfg) {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::fprintf(stderr, "[Viewer] glfwInit failed\n");
        return false;
    }

    // Core profile 3.3 
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

    // Load GL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "[Viewer] gladLoadGLLoader failed\n");
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    // VSync
    glfwSwapInterval(cfg.vsync ? 1 : 0);

    // Callbacks
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

    // Initial viewport
    glfwGetFramebufferSize(window_, &fbw_, &fbh_);
    glViewport(0, 0, fbw_, fbh_);

    // Basic GL state
    glEnable(GL_DEPTH_TEST);

    // compile shaders
    if(!solid_.compile(VS_LINES, FS_LINES)) return false;
    if (!meshLit_.compile(VS_MESH_LIT, FS_MESH_LIT)) {
        std::fprintf(stderr, "[Viewer] meshLit shader compile failed\n");
        return false;
    }

    // camera
    cam_.setViewport(fbw_, fbh_);
    cam_.setProj(60.f, 0.1f, 100000.f);

    // init geometry
    initGridAxes_();
    initDynamicVBOs_();
    initGround_();

    // basic mouse input
    glfwSetWindowUserPointer(window_, this);
    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double x, double y){
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if(!self) return;
        float dx = float(x - self->lastx_);
        float dy = float(y - self->lasty_);

        if (self->orbiting_) {
            self->cam_.orbit(dx, dy);          // pixels
        } else if (self->panning_) {
            self->cam_.pan(dx, dy);            // pixels
        }
        self->lastx_ = x; self->lasty_ = y;
    });

    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods){
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if(!self) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                self->orbiting_ = !(mods & GLFW_MOD_SHIFT); // LMB: orbit
                self->panning_  =  (mods & GLFW_MOD_SHIFT); // Shift+LMB: pan
                // Reset deltas on press to avoid jump
                double x,y; glfwGetCursorPos(w,&x,&y);
                self->lastx_ = x; self->lasty_ = y;
            } else if (action == GLFW_RELEASE) {
                self->orbiting_ = false; self->panning_ = false;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                self->panning_ = true;
                double x,y; glfwGetCursorPos(w,&x,&y);
                self->lastx_ = x; self->lasty_ = y;
            } else if (action == GLFW_RELEASE) {
                self->panning_ = false;
            }
        }
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* w, double , double yoff){
        auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
        if(!self) return;
        self->cam_.dolly(float(yoff)); // positive yoff zooms in
    });

    // Print GPU/GL info
    std::fprintf(stderr, "[Viewer] GL Vendor  : %s\n", glGetString(GL_VENDOR));
    std::fprintf(stderr, "[Viewer] GL Renderer: %s\n", glGetString(GL_RENDERER));
    std::fprintf(stderr, "[Viewer] GL Version : %s\n", glGetString(GL_VERSION));
    initialized_ = true; 
    return true;
    
}

bool Renderer::beginFrame() {
    if (!window_) return false;

    glfwPollEvents();
    glfwGetFramebufferSize(window_, &fbw_, &fbh_);
    glViewport(0,0, fbw_, fbh_);

    // Clear to dark gray
    glClearColor(0.08f, 0.09f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return true;
}

void Renderer::drawScene() {
    // recompute proj on resize
    cam_.setViewport(fbw_, fbh_);
    cam_.setProj(60.f, 0.1f, 100000.f);
    glm::mat4 view = cam_.view();
    glm::mat4 proj = cam_.proj();
    glm::mat4 vp   = proj * view;

    if (vao_ground_ != 0 && count_ground_ > 0) {
        meshLit_.use();

        glm::mat4 model(1.0f);
        meshLit_.setMat4("uModel", model);
        meshLit_.setMat4("uView",  view);
        meshLit_.setMat4("uProj",  proj);

        glm::vec3 lightPos(1500.0f, 1500.0f, 100.0f);

        meshLit_.setVec3("uLightPos", lightPos);
        float intensity = 1.2f;
        meshLit_.setFloat("uLightIntensity", intensity);
        meshLit_.setVec3("uBaseColor", glm::vec3(0.0f, 0.5f, 0.0f));

        glBindVertexArray(vao_ground_);
        glDrawArrays(GL_TRIANGLES, 0, count_ground_);
        glBindVertexArray(0);
    }


    solid_.use();
    solid_.setMat4("uVP", vp);

    // draw grid (grey)
    solid_.setVec3("uColor", {0.35f, 0.37f, 0.40f});
    glBindVertexArray(vao_grid_);
    //glDrawArrays(GL_LINES, 0, count_grid_);

    // draw axes (RGB)
    glBindVertexArray(vao_axes_);
    // X axis (red)
    solid_.setVec3("uColor", {0.9f,0.2f,0.2f}); glDrawArrays(GL_LINES, 0, 2);
    // Y axis (green)
    solid_.setVec3("uColor", {0.2f,0.9f,0.2f}); glDrawArrays(GL_LINES, 2, 2);
    // Z axis (blue)
    solid_.setVec3("uColor", {0.2f,0.4f,0.9f}); glDrawArrays(GL_LINES, 4, 2);
    glBindVertexArray(0);

    drainBus_();

    drawTrail_(vao_trail_m_, vbo_trail_m_, trail_m_, {0.0f, 1.0f, 0.0f});
    drawTrail_(vao_trail_t_, vbo_trail_t_, trail_t_, {1.0f, 0.0f, 0.0f});

    drawMarkerCross_(last_m_, 20.f, {0.0f, 1.0f, 0.0f});
    drawMarkerCross_(last_t_, 60.f, {1.0f, 0.0f, 0.0f});
}

void Renderer::endFrame() {
    if (!window_) return;
    glfwSwapBuffers(window_);
}

bool Renderer::shouldClose() const {
    return !window_ || glfwWindowShouldClose(window_);
}

void Renderer::shutdown() {
    // Idempotent guard (require: set initialized_ = true at end of init())
    if (!initialized_) return;
    initialized_ = false;

    // Stop consuming external data during teardown
    bus_ = nullptr;

    // Ensure THIS context is current before any glDelete* / shader destroy
    if (window_) {
        glfwMakeContextCurrent(window_);
    }

    // --- Delete GL resources while a context is alive ---
    // ground
    if (vao_ground_) { glDeleteVertexArrays(1, &vao_ground_); vao_ground_ = 0; }
    if (vbo_ground_) { glDeleteBuffers(1,       &vbo_ground_); vbo_ground_ = 0; }
    count_ground_ = 0;

    // grid
    if (vao_grid_)  { glDeleteVertexArrays(1, &vao_grid_);  vao_grid_  = 0; }
    if (vbo_grid_)  { glDeleteBuffers(1,       &vbo_grid_);  vbo_grid_  = 0; }
    count_grid_ = 0;

    // axes
    if (vao_axes_)  { glDeleteVertexArrays(1, &vao_axes_);  vao_axes_  = 0; }
    if (vbo_axes_)  { glDeleteBuffers(1,       &vbo_axes_);  vbo_axes_  = 0; }
    count_axes_ = 0;

    // trails
    if (vao_trail_m_) { glDeleteVertexArrays(1, &vao_trail_m_); vao_trail_m_ = 0; }
    if (vbo_trail_m_) { glDeleteBuffers(1,       &vbo_trail_m_); vbo_trail_m_ = 0; }

    if (vao_trail_t_) { glDeleteVertexArrays(1, &vao_trail_t_); vao_trail_t_ = 0; }
    if (vbo_trail_t_) { glDeleteBuffers(1,       &vbo_trail_t_); vbo_trail_t_ = 0; }

    // Shader/program teardown must happen while context is current
    meshLit_.destroy();
    solid_.destroy();  // no-op if not compiled; leaves program id = 0

    // --- Destroy the window LAST (kills the context) ---
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

// Z-up: grid on the XY plane (Z = 0), axes: X=red, Y=green, Z=blue
void Renderer::initGridAxes_() {
    // --- Grid (2 km x 2 km, 10 m spacing) ---
    const int   half = 100;   // 100 * 10 m each side => 2 km span
    const float step = 10.f;  // 10 meters
    std::vector<glm::vec3> grid;
    grid.reserve((half * 2 + 1) * 4);

    // Vertical lines (parallel to Y) at each X
    for (int i = -half; i <= half; ++i) {
        float x = i * step;
        grid.emplace_back(x, -half * step, 0.f);
        grid.emplace_back(x, +half * step, 0.f);
    }
    // Horizontal lines (parallel to X) at each Y
    for (int i = -half; i <= half; ++i) {
        float y = i * step;
        grid.emplace_back(-half * step, y, 0.f);
        grid.emplace_back(+half * step, y, 0.f);
    }

    count_grid_ = static_cast<GLuint>(grid.size());

    glGenVertexArrays(1, &vao_grid_);
    glGenBuffers(1, &vbo_grid_);
    glBindVertexArray(vao_grid_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_grid_);
    glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(glm::vec3), grid.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(/*index*/0, /*size*/3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // --- Axes (length 500 m) ---
    const float L = 500.f;
    // X (red), Y (green), Z (blue)
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(/*index*/0, /*size*/3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::initGround_() {
    // Simple big quad on Z=0, centered at origin.
    // Size: 4000m x 4000m just to give you a "floor".
    const float S = 4000.f;

    struct VertexPN {
        glm::vec3 pos;
        glm::vec3 normal;
    };

    glm::vec3 n(0.f, 0.f, 1.f); // Z-up normal
    VertexPN verts[6] = {
        // Triangle 1
        { { -S, -S, 0.f }, n },
        { {  S, -S, 0.f }, n },
        { {  S,  S, 0.f }, n },
        // Triangle 2
        { { -S, -S, 0.f }, n },
        { {  S,  S, 0.f }, n },
        { { -S,  S, 0.f }, n },
    };

    count_ground_ = 6;

    glGenVertexArrays(1, &vao_ground_);
    glGenBuffers(1, &vbo_ground_);

    glBindVertexArray(vao_ground_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ground_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN),
        (void*)offsetof(VertexPN, pos)
    );
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN),
        (void*)offsetof(VertexPN, normal)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void Renderer::initDynamicVBOs_() {
    glGenVertexArrays(1, &vao_trail_m_);
    glGenBuffers(1, &vbo_trail_m_);
    glBindVertexArray(vao_trail_m_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_trail_m_);
    glBufferData(GL_ARRAY_BUFFER, trail_cap_ * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glGenVertexArrays(1, &vao_trail_t_);
    glGenBuffers(1, &vbo_trail_t_);
    glBindVertexArray(vao_trail_t_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_trail_t_);
    glBufferData(GL_ARRAY_BUFFER, trail_cap_ * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::drainBus_() {
    if (!bus_) return;
    static std::vector<TelemetrySample> tmp;
    tmp.clear();
    bus_->drain(tmp, 16384);
    if (tmp.empty()) return;

    for (const auto& s : tmp) {
        glm::vec3 pm{s.mx, s.my, s.mz};
        glm::vec3 pt{s.tx, s.ty, s.tz};
        trail_m_.push_back(pm);
        trail_t_.push_back(pt);
        last_m_ = pm;
        last_t_ = pt;
    }

    // cap trail lengths
    auto clip = [&](std::vector<glm::vec3>& v) {
        if (v.size() > trail_cap_) {
            v.erase(v.begin(), v.begin() + (v.size() - trail_cap_));
        }
    };
    clip(trail_m_);
    clip(trail_t_);
}

void Renderer::drawTrail_(GLuint vao, GLuint vbo, const std::vector<glm::vec3>& pts, const glm::vec3& color) {
    if (pts.empty()) return;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // upload current points (bounded by trail_cap_)
    glBufferSubData(GL_ARRAY_BUFFER, 0, pts.size()*sizeof(glm::vec3), pts.data());
    solid_.setVec3("uColor", color);
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)pts.size());
    glBindVertexArray(0);
}

void Renderer::drawMarkerCross_(const glm::vec3& p, float L, const glm::vec3& color) {
    glm::vec3 lines[] = {
        {p.x - L, p.y,     p.z}, {p.x + L, p.y,     p.z}, // X
        {p.x,     p.y - L, p.z}, {p.x,     p.y + L, p.z}, // Y
        {p.x,     p.y,     p.z - L}, {p.x, p.y, p.z + L}  // Z
    };
    GLuint vao=0, vbo=0;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);

    solid_.setVec3("uColor", color);
    glDrawArrays(GL_LINES, 0, 6);

    glBindVertexArray(0);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
}

}