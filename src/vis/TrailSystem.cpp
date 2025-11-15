#include "vis/TrailSystem.hpp"

namespace vis {

TrailSystem::TrailSystem(std::size_t capacity)
    : capacity_(capacity)
{
    missileTrail_.reserve(capacity_);
    targetTrail_.reserve(capacity_);
}

void TrailSystem::setCapacity(std::size_t cap)
{
    capacity_ = cap;
    enforceCapacity_(missileTrail_);
    enforceCapacity_(targetTrail_);
}

void TrailSystem::clear()
{
    missileTrail_.clear();
    targetTrail_.clear();
    lastMissile_ = glm::vec3(0.0f);
    lastTarget_  = glm::vec3(0.0f);
}

void TrailSystem::addSample(const glm::vec3& missileEnu,
                            const glm::vec3& targetEnu)
{
    missileTrail_.push_back(missileEnu);
    targetTrail_.push_back(targetEnu);

    lastMissile_ = missileEnu;
    lastTarget_  = targetEnu;

    enforceCapacity_(missileTrail_);
    enforceCapacity_(targetTrail_);
}

void TrailSystem::enforceCapacity_(std::vector<glm::vec3>& trail)
{
    if (trail.size() > capacity_) {
        const auto excess = trail.size() - capacity_;
        trail.erase(trail.begin(), trail.begin() + static_cast<std::ptrdiff_t>(excess));
    }
}

} // namespace vis
