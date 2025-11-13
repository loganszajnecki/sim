#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vector>

#include "sim/Missile.hpp"
#include "geo/GeoTypes.hpp"
#include "geo/GeoUtils.hpp"

namespace sim {

/**
 * @brief Aggregates initial conditions and basic simulation parameters
 *        constructed from a JSON scenario file.
 *
 * All times are in seconds. The state vectors are expected to use the
 * current 3-DoF layout:
 *   x[0..2] : position (px, py, pz)
 *   x[3..5] : velocity (vx, vy, vz)
 */
struct FactoryParams {
    /// Initial missile state (size = Missile::Indices::Size).
    State missile0{Missile::Indices::Size};

    /// Initial target state (size = Missile::Indices::Size).
    State target0{Missile::Indices::Size};

    /// Start time [s].
    double t0{0.0};

    /// End time [s].
    double tf{60.0};

    /// Fixed time step [s].
    double h{0.01};

    /// Earth-anchored local ENU origin
    geo::GeoOrigin origin;
};

/**
 * @brief Parse a JSON scenario file and configure a Missile + parameters.
 *
 * The JSON file typically contains:
 *
 *   {
 *     "t0": 0.0,
 *     "tf": 30.0,
 *     "h":  0.01,
 *     "initial_state": {
 *       "pos": [px, py, pz],
 *       "vel": [vx, vy, vz]
 *     },
 *     "target": {
 *       "pos": [tx, ty, tz],
 *       "vel": [tvx, tvy, tvz]
 *     },
 *     "missile": {
 *       "aero": { "type": "SimpleAero", ... },
 *       "guidance": { "type": "ProNav", ... },
 *       "autopilot": { "type": "SimpleAccel", ... }
 *     }
 *   }
 *
 * On success:
 *   - `missile` will have its models (aero/guidance/autopilot) configured as
 *     requested in the file (or with defaults where specified).
 *   - `out` will contain initial missile and target states plus (t0, tf, h).
 *
 * On failure:
 *   - Returns false and writes a human-readable message into `error`.
 *   - `missile` and `out` may be partially configured.
 *
 * @param path   Filesystem path to the JSON configuration file.
 * @param missile Missile instance whose models will be configured.
 * @param out    Output structure holding initial conditions and sim params.
 * @param error  On failure, receives an error message.
 * @return       true on success, false on failure.
 */
bool load_from_json(const std::string& path,
                    Missile& missile,
                    FactoryParams& out,
                    std::string& error);

} // namespace sim
