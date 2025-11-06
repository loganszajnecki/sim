#include <vis/Camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace vis {

void Camera::setViewport(int w, int h) { width_ = w; height_ = h; }

void Camera::setProj(float fov_deg, float nearZ, float farZ) {
    fov_y_deg_ = fov_deg;
    proj_ = glm::perspective(glm::radians(fov_deg), aspect(), nearZ, farZ);
}

// Natural orbit: yaw about world Z, pitch about camera right
void Camera::orbit(float dx_pixels, float dy_pixels) {
    // Radians per pixel scaled to FOV/viewport height
    float rad_per_px = glm::radians(fov_y_deg_) / std::max(1, height_);
    float yaw_delta   = -dx_pixels * orbit_sensitivity_ * rad_per_px; // left->+yaw
    float pitch_delta = -dy_pixels * orbit_sensitivity_ * rad_per_px; // up->+pitch

    yaw_   = wrapAngle(yaw_ + yaw_delta);
    pitch_ = std::clamp(pitch_ + pitch_delta, -pitch_limit_, pitch_limit_);
}

// Screen-space pan: dx,dy in pixels
void Camera::pan(float dx_pixels, float dy_pixels) {
    // Convert pixels to world units roughly proportional to distance & FOV
    float world_per_px = 2.f * radius_ * std::tan(0.5f * glm::radians(fov_y_deg_))
                         / std::max(1, height_);
    glm::vec3 right = rightVector();
    glm::vec3 up    = worldUp_; // lock to world up for clean vertical pans
    target_ += (dx_pixels * world_per_px) * right
               + ( dy_pixels * world_per_px) * up;
}

// Exponential dolly: dr>0 zoom in, dr<0 zoom out
// e.g., call with mouse wheel "yoff"
void Camera::dolly(float scroll_steps) {
    // Each step scales distance by a constant factor
    float scale = std::exp(-scroll_steps * zoom_speed_);
    radius_ = std::clamp(radius_ * scale, 0.01f, max_radius_);
}

glm::mat4 Camera::view() const {
    // Build direction from yaw (around world Z) then pitch (around right)
    glm::vec3 dir = forwardFromYawPitch();
    glm::vec3 eye = target_ - dir * radius_;
    return glm::lookAt(eye, target_, worldUpFrom(dir));
}

glm::mat4 Camera::proj() const { return proj_; }

float Camera::aspect() const { return (height_ > 0) ? float(width_) / float(height_) : 16.f/9.f; }

float Camera::wrapAngle(float a) {
    // Keep yaw within [-pi, pi] for numerical niceness
    const float pi = glm::pi<float>();
    a = std::fmod(a + pi, 2.f * pi);
    if (a < 0) a += 2.f * pi;
    return a - pi;
}

glm::vec3 Camera::forwardFromYawPitch() const {
    // Start facing +X in world with Z-up convention: base dir (1,0,0)
    // Apply yaw around world Z, then pitch around camera's right
    // Construct from spherical-like but with world-up yaw:
    float cy = std::cos(yaw_),  sy = std::sin(yaw_);
    float cp = std::cos(pitch_), sp = std::sin(pitch_);
    // Yaw rotates base (1,0,0) to (cy, sy, 0); pitch tilts toward Z
    glm::vec3 f = glm::vec3(cp * cy, cp * sy, sp);
    return glm::normalize(f);
}

glm::vec3 Camera::rightVector() const {
    glm::vec3 f = forwardFromYawPitch();
    glm::vec3 r = glm::normalize(glm::cross(worldUp_, f));
    // If near singular (looking straight up/down), fall back:
    if (glm::dot(r, r) < 1e-6f) r = glm::vec3(1,0,0);
    return r;
}

glm::vec3 Camera::worldUpFrom(const glm::vec3& dir) const {
    // Keep "up" orthogonal to view to avoid roll surprises
    glm::vec3 r = glm::normalize(glm::cross(worldUp_, dir));
    if (glm::dot(r, r) < 1e-6f) return worldUp_; // straight up/down
    return glm::normalize(glm::cross(dir, r));
}

} // namespace vis
