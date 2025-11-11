#pragma once

#include <glm/glm.hpp>
#include "vis/models/TexturedModel.hpp"

namespace vis {

/**
 * @brief Represents a single renderable instance of a textured model.
 *
 * Each Entity corresponds to one instance of a model in the scene.
 * It stores only transform data (position, rotation, scale) and a
 * pointer to a shared TexturedModel, which holds the mesh and texture.
 *
 * Coordinate system:
 *  - World is Z-up (consistent with Renderer / Camera).
 *  - Rotation is applied in degrees, typically XYZ order.
 *  - Position units are meters.
 *
 * Ownership:
 *  - The Entity does *not* own its TexturedModel.
 *  - The caller (e.g. Loader or Renderer setup) must ensure that the
 *    referenced TexturedModel outlives the Entity.
 *
 * Example:
 *   TexturedModel treeModel = ...;
 *   Entity tree(&treeModel, {0,0,0}, {0,0,0}, 1.0f);
 *   tree.translate({10, 0, 0});
 *   tree.rotate({0, 90, 0});
 */
struct Entity {
    /// Non-owning pointer to shared model/texture data.
    TexturedModel* model = nullptr;

    /// World position (meters, Z-up).
    glm::vec3 position{0.0f};

    /// Euler rotation in degrees (rotX, rotY, rotZ).
    glm::vec3 rotation{0.0f};

    /// Uniform scale factor (1.0 = nominal size).
    float scale = 1.0f;

    Entity() = default;

    Entity(TexturedModel* m,
           const glm::vec3& pos,
           const glm::vec3& rot,
           float s)
        : model(m), position(pos), rotation(rot), scale(s) {}

    /// Translate the entity in world space by vector d (meters).
    void translate(const glm::vec3& d) { position += d; }

    /// Increment Euler rotation (degrees).
    void rotate(const glm::vec3& d) { rotation += d; }
};

} // namespace vis
