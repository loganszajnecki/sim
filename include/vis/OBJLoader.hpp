#pragma once

#include <string>
#include "vis/models/RawModel.hpp"
#include "vis/Loader.hpp"

namespace vis {

class OBJLoader {
public:
    // Loads "res/<fileName>.obj"
    static RawModel loadObjModel(const std::string& fileName,
                                 Loader& loader);
};

} // namespace vis
