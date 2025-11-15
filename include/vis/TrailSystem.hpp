#pragma once

#include <vector>
#include <cstddef>
#include <glm/glm.hpp>

namespace vis {

/**
 * @brief Stores missile and target trails in ENU space with a fixed capacity.
 *
 * Responsibilities:
 *  - Maintain rolling buffers of missile/target positions.
 *  - Track the last missile/target positions for camera follow.
 *  - Enforce a maximum capacity (oldest samples dropped).
 *
 * RAII:
 *  - All storage is owned; no external resources.
 */
class TrailSystem {
public:
    explicit TrailSystem(std::size_t capacity = 5000);

    /// Change capacity; will trim existing trails if needed.
    void setCapacity(std::size_t cap);

    /// Clear all stored samples.
    void clear();

    /// Add a new missile/target ENU sample.
    void addSample(const glm::vec3& missileEnu,
                   const glm::vec3& targetEnu);

    /// Accessors.
    const std::vector<glm::vec3>& missileTrail() const noexcept { return missileTrail_; }
    const std::vector<glm::vec3>& targetTrail() const noexcept { return targetTrail_; }

    const glm::vec3& lastMissile() const noexcept { return lastMissile_; }
    const glm::vec3& lastTarget()  const noexcept { return lastTarget_; }

    std::size_t capacity() const noexcept { return capacity_; }

private:
    void enforceCapacity_(std::vector<glm::vec3>& trail);

    std::vector<glm::vec3> missileTrail_;
    std::vector<glm::vec3> targetTrail_;
    glm::vec3              lastMissile_{0.0f, 0.0f, 0.0f};
    glm::vec3              lastTarget_{0.0f, 0.0f, 0.0f};
    std::size_t            capacity_;
};


} // namespace vis