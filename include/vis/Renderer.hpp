#pragma once
#include <string>
#include <glad/glad.h>
struct GLFWwindow;

#include <glm/glm.hpp>
#include <vis/Camera.hpp>
#include <vis/Shader.hpp>
#include "vis/Telemetry.hpp"

namespace vis {

struct RendererConfig {
    int width = 1920;
    int height = 1080;
    bool vsync = true;
    const char* title = "SAM Viewer";
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() { shutdown(); }

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    // allow move:
    Renderer(Renderer&&) = default;
    Renderer& operator=(Renderer&&) = default;

    bool init(const RendererConfig& cfg = {});
    bool beginFrame(); // polls events, clears framebuffer
    void drawScene();  // main draw routine
    void endFrame();   // swap buffers
    void shutdown();

    bool shouldClose() const; // window close state
    bool isInitialized() const { return initialized_; }
    void attachBus(TelemetryBus* bus) { bus_ = bus; }
private:
    bool initialized_ = false; 
    
    void initGridAxes_();
    void initDynamicVBOs_();
    void drainBus_();
    void drawTrail_(GLuint vao, GLuint vbo, const std::vector<glm::vec3>& pts, const glm::vec3& color);
    void drawMarkerCross_(const glm::vec3& p, float L, const glm::vec3& color);

    GLFWwindow* window_ = nullptr;
    int fbw_ = 0, fbh_ = 0; // framebuffer size (for viewport)

    // cam and shaders
    Camera cam_;
    Shader solid_;

    // simple VBO/VAO for lines
    GLuint vao_grid_ = 0, vbo_grid_ = 0, count_grid_ = 0;
    GLuint vao_axes_ = 0, vbo_axes_ = 0, count_axes_ = 0;

    // dynamic trails
    GLuint vao_trail_m_ = 0, vbo_trail_m_ = 0;
    GLuint vao_trail_t_ = 0, vbo_trail_t_ = 0;
    size_t trail_cap_ = 5000;

    std::vector<glm::vec3> trail_m_;
    std::vector<glm::vec3> trail_t_;
    glm::vec3 last_m_{0.f}, last_t_{0.f};

    TelemetryBus* bus_ = nullptr;

    // input state
    bool orbiting_ = false;   // LMB drag
    bool panning_  = false;   // MMB drag or Shift+LMB
    double lastx_ = 0.0, lasty_ = 0.0;
};

} // namespace vis
