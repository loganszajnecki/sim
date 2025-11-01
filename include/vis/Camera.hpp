#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vis {

class Camera
{
public:
    // Orbit camera looking at origin
    void setViewport(int w, int h) { width_= w; height_= h; }
    void setProj(float fov_deg = 60.0f, float nearZ = 0.1f, float farZ = 50000.f) {
        proj_ = glm::perspective(glm::radians(fov_deg), aspect(), nearZ, farZ);
    }
    void orbit(float dtheta, float dphi) {
        theta_ += dtheta;
        phi_ += dphi;
        const float eps = 0.001f;
        if (phi_ < eps) {
            phi_ = eps;
        }
        if (phi_ > glm::pi<float>() - eps) {
            phi_ = glm::pi<float>() - eps;
        }
    }

    void dolly(float dr) { radius_ = glm::max(0.1f, radius_ * (1.f - dr)); }

    glm::mat4 view() const {
        // spherical to cartesian
        float x = radius_ * sinf(phi_) * cosf(theta_);
        float y = radius_ * cosf(phi_);
        float z = radius_ * sinf(phi_) * sinf(theta_);
        return glm::lookAt(glm::vec3(x,y,z) + target_, target_, up_);
    }
    glm::mat4 proj() const { return proj_; }
private:
    float aspect() const { return (height_ > 0) ? float(width_)/float(height_) : 16.f/9.f; }
    int width_ = 1920, height_ = 1080;
    glm::mat4 proj_{.1f};
    glm::vec3 target_{0.f, 0.f, 0.f}, up_{0.f, 0.f, 1.f}; // Z up
    float radius_ = 3000.f; // meters
    float theta_ = 0.9f;
    float phi_ = 1.0f;
};

} // namespace vis