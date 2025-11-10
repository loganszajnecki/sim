#pragma once
#include "vis/models/RawModel.hpp"
#include "vis/models/ModelTexture.hpp"

namespace vis {

struct TexturedModel {
    RawModel     raw;
    ModelTexture texture;

    TexturedModel() = default;

    TexturedModel(const RawModel& r,
                  const ModelTexture& t)
        : raw(r), texture(t) {}
};

} // namespace vis
