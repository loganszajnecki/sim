#pragma once

#include <unordered_map>
#include <vector>

#include <glad/glad.h>

#include "vis/shaders/EntityShader.hpp"
#include "vis/models/TexturedModel.hpp"
#include "vis/entities/Entity.hpp"

namespace vis {

/**
 * @brief Low-level renderer for batched textured entities.
 *
 * Responsibilities:
 *  - Given a batch map of TexturedModel* -> list<Entity*>, it:
 *      * Binds each model's VAO + texture.
 *      * Configures culling based on texture transparency.
 *      * Uploads per-instance transform matrices.
 *      * Issues glDrawElements calls.
 *
 * Ownership:
 *  - EntityRenderer does not own TexturedModel or Entity objects.
 *  - The caller must ensure those objects outlive the render() call.
 *
 * Usage:
 *
 *   EntityRenderer renderer(shader);
 *   EntityRenderer::BatchMap batches;
 *   batches[&model].push_back(&entity1);
 *   batches[&model].push_back(&entity2);
 *   ...
 *   renderer.render(batches);
 */
class EntityRenderer
{
public:
    explicit EntityRenderer(EntityShader& shader);

    /// key: TexturedModel*, value: list of Entity* instances
    using BatchMap = std::unordered_map<TexturedModel*, std::vector<Entity*>>;

    /**
     * @brief Render all entities in the given batch map.
     *
     * For each distinct TexturedModel, this will:
     *  - Bind the model's VAO and attributes (0: position, 1: texcoords, 2: normals).
     *  - Bind the model's texture and configure culling based on transparency.
     *  - For each Entity*, upload its transform and draw its indexed mesh.
     *
     * Must be called with:
     *  - A valid OpenGL context current.
     *  - shader_ already started (shader_.start()) and uniforms like
     *    projection/view set by the caller (e.g. via MasterRenderer).
     */
    void render(const BatchMap& entities);
    
private:
    /// Bind VAO, attributes, texture, and material uniforms for a textured model.
    void prepareTexturedModel(TexturedModel* model);

    /// Reset GL state (attributes, VAO, culling) after drawing a model batch.
    void unbindTexturedModel();

    /// Upload the model matrix for a single entity into the shader.
    void prepareInstance(const Entity& entity);

private:
    EntityShader& shader_;
};

} // namespace vis
