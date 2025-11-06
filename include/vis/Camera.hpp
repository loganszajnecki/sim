#pragma once
#include <glm/glm.hpp>

namespace vis {

class Camera {
public:
    void setViewport(int w, int h);
    void setProj(float fov_deg = 60.f, float nearZ = 0.1f, float farZ = 50000.f);

    // Natural orbit: yaw about world Z, pitch about camera right
    void orbit(float dx_pixels, float dy_pixels);

    // Screen-space pan: dx,dy in pixels
    void pan(float dx_pixels, float dy_pixels);

    // Exponential dolly: dr>0 zoom in, dr<0 zoom out
    // e.g., call with mouse wheel "yoff"
    void dolly(float scroll_steps);

    glm::mat4 view() const;
    glm::mat4 proj() const;

private:
    float aspect() const;
    static float wrapAngle(float a);
    glm::vec3 forwardFromYawPitch() const;
    glm::vec3 rightVector() const;
    glm::vec3 worldUpFrom(const glm::vec3& dir) const;

private:
    int width_ = 1920, height_ = 1080;
    glm::mat4 proj_{1.0f};
    float fov_y_deg_ = 60.f;

    glm::vec3 target_{0.f, 0.f, 0.f};
    const glm::vec3 worldUp_{0.f, 0.f, 1.f}; // Z-up

    float radius_ = 3000.f;
    float max_radius_ = 1e7f;

    // Yaw (around world Z) and pitch (around right)
    float yaw_   = 0.9f;
    float pitch_ = 0.2f; // around 11 degrees

    // Limits to avoid pole weirdness (almost ±90°)
    const float pitch_limit_ = glm::radians(89.0f);

    // Tunables
    float orbit_sensitivity_ = 1.0f; // 1.0 feels good with FOV scaling
    float zoom_speed_ = 0.15f;       // exp zoom factor per wheel "step"
};

} // namespace vis
