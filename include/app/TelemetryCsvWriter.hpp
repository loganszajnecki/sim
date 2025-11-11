#pragma once

#include <fstream>
#include <string>

namespace app {

/**
 * @brief Simple CSV writer for missile/target telemetry.
 *
 * Output format (header row):
 *   t,px,py,pz,vx,vy,vz,tx,ty,tz
 *
 * Assumes the state-like type S has a member:
 *   - std::vector<double> x with at least 6 elements:
 *       x[0..2] : position (px, py, pz)
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
            out_ << "t,px,py,pz,vx,vy,vz,tx,ty,tz\n";
        }
    }

    /// @return true if the underlying stream is open and ready.
    [[nodiscard]] bool good() const noexcept { return out_.good(); }

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

        out_ << t << ","
             << m.x[0] << "," << m.x[1] << "," << m.x[2] << ","
             << m.x[3] << "," << m.x[4] << "," << m.x[5] << ","
             << tgt.x[0] << "," << tgt.x[1] << "," << tgt.x[2];

        if (line_flush_) {
            out_ << std::endl; // flushes if unitbuf is unset
        } else {
            out_ << '\n';
        }
    }

    /// Manually flush the underlying stream.
    void flush() { out_.flush(); }

private:
    std::ofstream out_;
    bool          line_flush_{true};
};

} // namespace app
