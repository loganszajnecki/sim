#pragma once

#include <fstream>
#include <string>
#include <glm/glm.hpp>

#include "geo/GeoTypes.hpp"
#include "geo/GeoUtils.hpp"

namespace app {

/**
 * @brief Simple CSV writer for missile/target telemetry.
 *
 * Output format (header row):
 *   t,px,py,pz,vx,vy,vz,tx,ty,tz,lat_deg,lon_deg,alt_m
 *
 * Assumes the state-like type S has a member:
 *   - std::vector<double> x with at least 6 elements:
 *       x[0..2] : position (px, py, pz) in local ENU (m)
 *       x[3..5] : velocity (vx, vy, vz)
 */
class TelemetryCsvWriter {
public:
    /**
     * @param path        Filesystem path to the CSV file.
     * @param line_flush  If true, flush after every line (via std::endl).
     *                    If false, use '\n' and rely on manual flush() calls.
     */
    explicit TelemetryCsvWriter(const std::string& path,
                                bool line_flush = true)
        : out_(path, std::ios::out)
        , line_flush_(line_flush)
    {
        if (out_) {
            if (line_flush_) {
                // unitbuf: flush after each insertion
                out_.setf(std::ios::unitbuf);
            }
            // Always include LLA columns in the header.
            out_ << "t,px,py,pz,vx,vy,vz,tx,ty,tz,lat_deg,lon_deg,alt_m\n";
        }
    }

    /// @return true if the underlying stream is open and ready.
    [[nodiscard]] bool good() const noexcept { return out_.good(); }

    /// Set the geo origin used to convert ENU position to lat/lon/alt.
    /// Call this after loading the scenario but before the first write.
    void setOrigin(const geo::GeoOrigin* origin) { origin_ = origin; }

    /**
     * @brief Write a single telemetry sample (missile + target) to CSV.
     *
     * @tparam S  State-like type with member `x` (indexable with [0..5]).
     * @param t   Simulation time [s].
     * @param m   Missile state-like object.
     * @param tgt Target state-like object.
     */
    template <class S>
    void write(double t, const S& m, const S& tgt)
    {
        if (!out_) {
            return; // silently ignore if file couldn't be opened
        }

        const double px = m.x[0];
        const double py = m.x[1];
        const double pz = m.x[2];

        out_ << t << ","
             << px << "," << py << "," << pz << ","
             << m.x[3] << "," << m.x[4] << "," << m.x[5] << ","
             << tgt.x[0] << "," << tgt.x[1] << "," << tgt.x[2];

        double lat_deg = 0.0;
        double lon_deg = 0.0;
        double alt_m   = 0.0;

        if (origin_) {
            glm::dvec3 enu(px, py, pz);
            geo::GeoLLA lla = geo::llaFromENU(enu, *origin_);
            lat_deg = lla.lat_deg;
            lon_deg = lla.lon_deg;
            alt_m   = lla.alt_m;
        }

        out_ << "," << lat_deg << "," << lon_deg << "," << alt_m;

        if (line_flush_) {
            out_ << std::endl; // flushes if unitbuf is unset
        } else {
            out_ << '\n';
        }
    }

    /// Manually flush the underlying stream.
    void flush() { out_.flush(); }

private:
    std::ofstream         out_;
    const geo::GeoOrigin* origin_ {nullptr};
    bool                  line_flush_ {true};
};

} // namespace app
