#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vis {

struct MathUtils {
    // Build model transform: translate → rotate → scale
    static glm::mat4 createModelMatrix(
        const glm::vec3& translation,
        const glm::vec3& rotation_deg,
        float scale)
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, translation);
        m = glm::rotate(m, glm::radians(rotation_deg.x), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(rotation_deg.y), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(rotation_deg.z), glm::vec3(0, 0, 1));
        m = glm::scale(m, glm::vec3(scale));
        return m;
    }
};

} // namespace vis
