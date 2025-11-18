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

    // Enable/disable panning (used by Renderer depending on view mode).
    void setPanEnabled(bool enabled) { panEnabled_ = enabled; }

    // Allow or disallow follow logic entirely.
    void setFollowAllowed(bool allowed) { followAllowed_ = allowed; }

    // Force the follow flag on/off (used when switching modes).
    void forceFollow(bool enabled) { followEnabled_ = enabled; }

private:
    Camera* cam_{nullptr};   // non-owning

    bool   orbiting_{false};
    bool   panning_{false};
    double lastx_{0.0};
    double lasty_{0.0};

    bool followEnabled_{true};
    bool fPrevDown_{false};
    bool panEnabled_{true};    // can we pan at all?
    bool followAllowed_{true}; // can we even use follow (e.g. only in Local)?
};

} // namespace vis
