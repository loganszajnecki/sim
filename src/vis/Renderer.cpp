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

} // namespace

namespace vis {

Renderer::~Renderer() { shutdown(); }

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

    // compile shader
    if(!solid_.compile(VS_LINES, FS_LINES)) return false;

    // camera
    cam_.setViewport(fbw_, fbh_);
    cam_.setProj(60.f, 0.1f, 100000.f);

    // init geometry
    initGridAxes_();

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
    glm::mat4 vp = cam_.proj() * cam_.view();

    solid_.use();
    solid_.setMat4("uVP", vp);

    // draw grid (grey)
    solid_.setVec3("uColor", {0.35f, 0.37f, 0.40f});
    glBindVertexArray(vao_grid_);
    glDrawArrays(GL_LINES, 0, count_grid_);

    // draw axes (RGB)
    glBindVertexArray(vao_axes_);
    // X axis (red)
    solid_.setVec3("uColor", {0.9f,0.2f,0.2f});
    glDrawArrays(GL_LINES, 0, 2);
    // Y axis (green)
    solid_.setVec3("uColor", {0.2f,0.9f,0.2f});
    glDrawArrays(GL_LINES, 2, 2);
    // Z axis (blue)
    solid_.setVec3("uColor", {0.2f,0.4f,0.9f});
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
}

void Renderer::endFrame() {
    if (!window_) return;
    glfwSwapBuffers(window_);
}

bool Renderer::shouldClose() const {
    return !window_ || glfwWindowShouldClose(window_);
}

void Renderer::shutdown() {
    if (vao_grid_) glDeleteVertexArrays(1, &vao_grid_);
    if (vbo_grid_) glDeleteBuffers(1, &vbo_grid_);
    if (vao_axes_) glDeleteVertexArrays(1, &vao_axes_);
    if (vbo_axes_) glDeleteBuffers(1, &vbo_axes_);
    vao_grid_= vbo_grid_ = vao_axes_ = vbo_axes_ = 0;

    if(window_){ glfwDestroyWindow(window_); window_=nullptr; }
    if(glfwInit()){ glfwTerminate(); }
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

}