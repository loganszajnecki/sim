#pragma once

#include <glm/glm.hpp>
#include "vis/models/TexturedModel.hpp"


namespace vis {

struct Entity {
    TexturedModel* model = nullptr;
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // rotX, rotY, rotZ in degrees
    float scale = 1.0f;

    Entity() = default;

    Entity(TexturedModel* m,
           const glm::vec3& pos,
           const glm::vec3& rot,
           float s)
        : model(m), position(pos), rotation(rot), scale(s) {}

    void translate(const glm::vec3& d) { position += d; }
    void rotate(const glm::vec3& d) { rotation += d; }
};

} // namespace vis