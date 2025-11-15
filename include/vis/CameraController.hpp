#pragma once

#include <glm/glm.hpp>

struct GLFWwindow; // forward-declare to avoid heavy include

namespace vis {

class Camera;

/**
 * @brief Handles camera interaction: orbit, pan, zoom, and follow mode.
 *
 * RAII:
 *  - Pure state holder, no GL resources.
 *  - Non-owning pointer to a Camera that must outlive this controller.
 */
class CameraController {
public:
    CameraController() = default;

    /// Attach a camera to control (non-owning).
    void attachCamera(Camera* cam) { cam_ = cam; }

    /// Mouse move callback (screen-space coords).
    void onMouseMove(double x, double y);

    /// Mouse button callback, with cursor position at the moment of the event.
    void onMouseButton(int button, int action, int mods,
                       double x, double y);

    /// Scroll wheel callback (y offset).
    void onScroll(double yoff);

    /// Update follow toggle (checks F key state).
    void updateFollowToggle(GLFWwindow* window);

    /// Set the target the camera should follow (if follow is enabled).
    void setFollowTarget(const glm::vec3& worldPos);

    bool followEnabled() const noexcept { return followEnabled_; }

private:
    Camera* cam_{nullptr};   // non-owning

    bool   orbiting_{false};
    bool   panning_{false};
    double lastx_{0.0};
    double lasty_{0.0};

    bool followEnabled_{true};
    bool fPrevDown_{false};
};

} // namespace vis
