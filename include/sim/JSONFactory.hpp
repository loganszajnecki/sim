#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vector>
#include "sim/Missile.hpp"

namespace sim {

struct FactoryParams {
    // missile initial state
    State missile0; // size 6
    // target initial state
    State target0;
    double t0 = 0.0;
    double tf = 60.0;
    double h = 0.01;
};

// Parse JSON path and construct factory params
// returns false and fulls error on failure
bool load_from_json(const std::string& path, Missile& missile, FactoryParams& out, std::string& error);

} // namespace sim