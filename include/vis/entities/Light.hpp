#pragma once
#include <glm/glm.hpp>

namespace vis {

struct Light {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
};

} // namespace vis