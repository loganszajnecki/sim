#pragma once
#include <string>
#include <glad/glad.h>
struct GLFWwindow;

#include <glm/glm.hpp>
#include <vis/Camera.hpp>
#include <vis/Shader.hpp>

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
    ~Renderer();

    bool init(const RendererConfig& cfg = {});
    bool beginFrame(); // polls events, clears framebuffer
    void drawScene();  // main draw routine
    void endFrame();   // swap buffers
    void shutdown();

    bool shouldClose() const; // window close state

private:
    void initGridAxes_();
    void updateCameraFromInput_();

    GLFWwindow* window_ = nullptr;
    int fbw_ = 0, fbh_ = 0; // framebuffer size (for viewport)

    // cam and shaders
    Camera cam_;
    Shader solid_;

    // simple VBO/VAO for lines
    GLuint vao_grid_ = 0, vbo_grid_ = 0, count_grid_ = 0;
    GLuint vao_axes_ = 0, vbo_axes_ = 0, count_axes_ = 0;

    // input state
    bool dragging_ = false;
    double lastx_ = 0.0, lasty_ = 0.0;
};

} // namespace vis
