#include "vis/CameraController.hpp"

#include <GLFW/glfw3.h>
#include "vis/entities/Camera.hpp"

namespace vis {

void CameraController::onMouseMove(double x, double y)
{
    if (!cam_) return;

    float dx = static_cast<float>(x - lastx_);
    float dy = static_cast<float>(y - lasty_);

    if (orbiting_) {
        // Orbit always allowed (view mode gating is done in Renderer).
        cam_->orbit(dx, dy);
    } else if (panning_) {
        // Only pan if panning is enabled for the current view mode.
        if (panEnabled_) {
            cam_->pan(dx, dy);
        }
    }

    lastx_ = x;
    lasty_ = y;
}

void CameraController::onMouseButton(int button, int action, int mods,
                                     double x, double y)
{
    if (!cam_) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // LMB: orbit, Shift+LMB: pan.
            const bool wantPan   = (mods & GLFW_MOD_SHIFT) != 0;
            const bool wantOrbit = !wantPan;

            orbiting_ = wantOrbit;
            panning_  = wantPan && panEnabled_;

            // Reset deltas on press to avoid jump.
            lastx_ = x;
            lasty_ = y;
        } else if (action == GLFW_RELEASE) {
            orbiting_ = false;
            panning_  = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            // MMB: pan (if enabled).
            orbiting_ = false;
            panning_  = panEnabled_;
            lastx_    = x;
            lasty_    = y;
        } else if (action == GLFW_RELEASE) {
            panning_ = false;
        }
    }
}

void CameraController::onScroll(double yoff)
{
    if (!cam_) return;
    cam_->dolly(static_cast<float>(yoff)); // positive zooms in
}

void CameraController::updateFollowToggle(GLFWwindow* window)
{
    if (!cam_ || !window) return;

    // If follow is not allowed in this mode, force it off and ignore key.
    if (!followAllowed_) {
        followEnabled_ = false;
        fPrevDown_     = false;
        return;
    }

    const int  cur  = glfwGetKey(window, GLFW_KEY_F);
    const bool down = (cur == GLFW_PRESS);
    if (down && !fPrevDown_) {
        followEnabled_ = !followEnabled_;
    }
    fPrevDown_ = down;
}

void CameraController::setFollowTarget(const glm::vec3& worldPos)
{
    if (!cam_)           return;
    if (!followAllowed_) return;
    if (!followEnabled_) return;

    cam_->setTarget(worldPos);
}

} // namespace vis
