#pragma once

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vis {

/**
 * @brief Z-up orbit camera for interactive 3D viewing.
 *
 * Features:
 *  - Z-up world convention.
 *  - Orbit: yaw around world Z, pitch around camera right vector.
 *  - Pan: screen-space pan using pixels, scaled with distance & FOV.
 *  - Dolly: exponential zoom in/out using a radius around the target.
 */
class Camera {
public:
    /// Set viewport size in pixels (used for aspect and input scaling).
    void setViewport(int w, int h) { width_ = w; height_ = h; }

    /// Set perspective projection parameters.
    void setProj(float fov_deg = 60.f,
                 float nearZ   = 0.1f,
                 float farZ    = 50000.f)
    {
        fov_y_deg_ = fov_deg;
        proj_ = glm::perspective(glm::radians(fov_deg), aspect(), nearZ, farZ);
    }

    /**
     * @brief Orbit the camera around the target in response to mouse drag.
     */
    void orbit(float dx_pixels, float dy_pixels) {
        // Radians per pixel scaled to FOV/viewport height
        const float rad_per_px = glm::radians(fov_y_deg_) / std::max(1, height_);
        const float yaw_delta   = -dx_pixels * orbit_sensitivity_ * rad_per_px; // left->+yaw
        const float pitch_delta = -dy_pixels * orbit_sensitivity_ * rad_per_px; // up->+pitch

        yaw_   = wrapAngle(yaw_ + yaw_delta);
        pitch_ = std::clamp(pitch_ + pitch_delta, -pitch_limit_, pitch_limit_);
    }

    /**
     * @brief Pan in screen space (dx, dy in pixels).
     */
    void pan(float dx_pixels, float dy_pixels) {
        // Convert pixels to world units roughly proportional to distance & FOV
        const float world_per_px =
            2.f * radius_ * std::tan(0.5f * glm::radians(fov_y_deg_))
            / std::max(1, height_);

        const glm::vec3 right = rightVector();
        const glm::vec3 up    = worldUp_; // lock to world up for clean vertical pans

        target_ += (dx_pixels * world_per_px) * right
                 + (dy_pixels * world_per_px) * up;
    }

    /**
     * @brief Exponential dolly: positive scroll_steps zooms in, negative out.
     */
    void dolly(float scroll_steps) {
        const float scale = std::exp(-scroll_steps * zoom_speed_);
        radius_ = std::clamp(radius_ * scale, 0.01f, max_radius_);
    }

    /// View matrix (glm::lookAt) for current orbit state.
    glm::mat4 view() const {
        const glm::vec3 dir = forwardFromYawPitch();
        const glm::vec3 eye = target_ - dir * radius_;
        return glm::lookAt(eye, target_, worldUpFrom(dir));
    }

    /// Projection matrix (set via setProj()).
    glm::mat4 proj() const { return proj_; }

    /// Current camera world-space position (eye).
    glm::vec3 position() const {
        const glm::vec3 dir = forwardFromYawPitch();
        return target_ - dir * radius_;
    }

    /// Current camera target point.
    const glm::vec3& target() const noexcept { return target_; }

    /// Set camera target point.
    void setTarget(const glm::vec3& t) {
        target_ = t;
        // view() is recomputed on demand from yaw/pitch/radius/target.
    }

    /// Set camera world-space position, keeping the camera looking at current target.
    void setPosition(const glm::vec3& eye) {
        glm::vec3 toTarget = target_ - eye;
        float dist = glm::length(toTarget);
        if (dist < 1e-4f) {
            return; // degenerate; ignore
        }

        radius_ = std::clamp(dist, 0.01f, max_radius_);

        glm::vec3 dir = glm::normalize(toTarget);

        // For Z-up: dir = (cp*cy, cp*sy, sp)
        // pitch = asin(z), yaw = atan2(y, x)
        float sp = dir.z;
        sp = std::clamp(sp, -1.0f, 1.0f);
        float pitch = std::asin(sp);
        float cp    = std::cos(pitch);

        float yaw = 0.0f;
        if (cp > 1e-6f) {
            yaw = std::atan2(dir.y, dir.x);
        }

        yaw_   = wrapAngle(yaw);
        pitch_ = std::clamp(pitch, -pitch_limit_, pitch_limit_);
    }

    /// Set camera distance from target (clamped).
    void setRadius(float r) { radius_ = std::clamp(r, 0.01f, max_radius_); }

    /// Set yaw/pitch in radians directly, respecting pitch limits.
    void setYawPitch(float yaw, float pitch) {
        yaw_   = wrapAngle(yaw);
        pitch_ = std::clamp(pitch, -pitch_limit_, pitch_limit_);
    }

    // Keep the camera eye outside a sphere (e.g., the Earth),
    // while preserving the current target and view direction as much as possible.
    void ensureOutsideSphere(const glm::vec3& sphereCenter,
                             float sphereRadius,
                             float margin = 10.0f) // margin in world units
    {
        // Current forward direction and eye position.
        const glm::vec3 dir = forwardFromYawPitch();
        const glm::vec3 eye = target_ - dir * radius_;

        const glm::vec3 rel  = eye - sphereCenter;
        const float dist     = glm::length(rel);
        const float minDist  = sphereRadius + margin;

        // If we're already outside, nothing to do.
        if (dist >= minDist || dist < 1e-4f) {
            return;
        }

        // Push eye out along the radial direction from the sphere center.
        const glm::vec3 newRel = rel * (minDist / dist);
        const glm::vec3 newEye = sphereCenter + newRel;

        // Recompute radius so that we still look at the same target.
        const float newRadius = glm::length(target_ - newEye);

        radius_ = std::clamp(newRadius, 0.01f, max_radius_);
    }

private:
    float aspect() const {
        return (height_ > 0) ? float(width_) / float(height_) : 16.f / 9.f;
    }

    static float wrapAngle(float a) {
        // Keep yaw within [-pi, pi] for numerical niceness
        const float pi = glm::pi<float>();
        a = std::fmod(a + pi, 2.f * pi);
        if (a < 0) a += 2.f * pi;
        return a - pi;
    }

    glm::vec3 forwardFromYawPitch() const {
        // Start facing +X in world with Z-up convention: base dir (1,0,0)
        // Apply yaw around world Z, then pitch toward Z.
        const float cy = std::cos(yaw_),  sy = std::sin(yaw_);
        const float cp = std::cos(pitch_), sp = std::sin(pitch_);
        glm::vec3 f(cp * cy, cp * sy, sp);
        return glm::normalize(f);
    }

    glm::vec3 rightVector() const {
        const glm::vec3 f = forwardFromYawPitch();
        glm::vec3 r = glm::normalize(glm::cross(worldUp_, f));
        // If near singular (looking straight up/down), fall back:
        if (glm::dot(r, r) < 1e-6f) {
            r = glm::vec3(1, 0, 0);
        }
        return r;
    }

    glm::vec3 worldUpFrom(const glm::vec3& dir) const {
        // Keep "up" orthogonal to view to avoid roll surprises
        glm::vec3 r = glm::normalize(glm::cross(worldUp_, dir));
        if (glm::dot(r, r) < 1e-6f) {
            return worldUp_; // straight up/down
        }
        return glm::normalize(glm::cross(dir, r));
    }

private:
    int   width_  = 1920;
    int   height_ = 1080;

    glm::mat4 proj_{1.0f};
    float fov_y_deg_ = 60.f;

    glm::vec3 target_{0.f, 0.f, 0.f};
    const glm::vec3 worldUp_{0.f, 0.f, 1.f}; // Z-up

    float radius_     = 3000.f;
    float max_radius_ = 1e7f;

    // Yaw (around world Z) and pitch (around right), in radians.
    float yaw_   = 0.9f;
    float pitch_ = 0.2f; // ~11 degrees

    // Limits to avoid pole weirdness (almost ±90°)
    const float pitch_limit_ = glm::radians(89.0f);

    // Tunables
    float orbit_sensitivity_ = 1.0f; // 1.0 feels good with FOV scaling
    float zoom_speed_        = 0.15f; // exp zoom factor per wheel "step"
};

} // namespace vis
