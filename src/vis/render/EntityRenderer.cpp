#include "vis/render/EntityRenderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vis/MathUtils.hpp"

namespace vis {

EntityRenderer::EntityRenderer(EntityShader& shader)
    : shader_(shader)
{
    // Projection is loaded outside (from MasterRenderer / setup code).
}

void EntityRenderer::render(const BatchMap& entities)
{
    for (auto& kv : entities) {
        TexturedModel* model = kv.first;
        const auto&    batch = kv.second;

        if (!model) {
            continue;
        }

        prepareTexturedModel(model);

        for (const Entity* e : batch) {
            if (!e) {
                continue;
            }
            prepareInstance(*e);
            glDrawElements(GL_TRIANGLES,
                           model->raw.indexCount,
                           GL_UNSIGNED_INT,
                           (void*)0);
        }

        unbindTexturedModel();
    }
}

void EntityRenderer::prepareTexturedModel(TexturedModel* model)
{
    const RawModel& raw = model->raw;
    glBindVertexArray(raw.vao);

    glEnableVertexAttribArray(0); // position
    glEnableVertexAttribArray(1); // tex coords
    glEnableVertexAttribArray(2); // normals

    ModelTexture& tex = model->texture;

    // Transparency handling: disable back-face culling for transparent models
    // to avoid "missing" interior faces.
    if (tex.hasTransparency) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    shader_.loadFakeLighting(tex.useFakeLighting);
    shader_.loadShine(tex.shineDamper, tex.reflectivity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);
}

void EntityRenderer::unbindTexturedModel()
{
    // Restore default culling.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
}

void EntityRenderer::prepareInstance(const Entity& entity)
{
    const glm::mat4 model = MathUtils::createModelMatrix(
        entity.position,
        entity.rotation,
        entity.scale
    );
    shader_.loadTransformation(model);
}

} // namespace vis
