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
        cam_->orbit(dx, dy);   // pixels
    } else if (panning_) {
        cam_->pan(dx, dy);     // pixels
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
            orbiting_ = !(mods & GLFW_MOD_SHIFT);
            panning_  =  (mods & GLFW_MOD_SHIFT);
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
            panning_ = true;
            lastx_   = x;
            lasty_   = y;
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

    const int cur   = glfwGetKey(window, GLFW_KEY_F);
    const bool down = (cur == GLFW_PRESS);
    if (down && !fPrevDown_) {
        followEnabled_ = !followEnabled_;
    }
    fPrevDown_ = down;
}

void CameraController::setFollowTarget(const glm::vec3& worldPos)
{
    if (!cam_ || !followEnabled_) return;
    cam_->setTarget(worldPos);
}

} // namespace vis
