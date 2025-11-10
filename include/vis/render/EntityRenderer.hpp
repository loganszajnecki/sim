#pragma once

#include <unordered_map>
#include <vector>
#include <glad/glad.h>

#include "vis/shaders/EntityShader.hpp"
#include "vis/models/TexturedModel.hpp"
#include "vis/entities/Entity.hpp"

namespace vis {

class EntityRenderer
{
public:
    explicit EntityRenderer(EntityShader& shader);

    // key: TexturedModel*, value: list of Entity* instances
    using BatchMap = std::unordered_map<TexturedModel*, std::vector<Entity*>>;

    void render(const BatchMap& entities);
    
private:
    void prepareTexturedModel(TexturedModel* model);
    void unbindTexturedModel();
    void prepareInstance(const Entity& entity);

private:
    EntityShader& shader_;
};

} // namespace vis
