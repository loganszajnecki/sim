#pragma once

#include <deque>
#include <mutex>
#include <vector>
#include <cstddef>

namespace vis {

/**
 * @brief Telemetry sample for a single simulation step.
 *
 * Units:
 *   - Positions: meters (Z-up).
 *   - Velocities: meters/second.
 *   - Accels: meters/second^2 (optional, may be zero).
 */
struct TelemetrySample {
    double t{0.0};      ///< Simulation time [s].

    // Missile position.
    float mx{0.0f};
    float my{0.0f};
    float mz{0.0f};

    // Missile velocity.
    float mvx{0.0f};
    float mvy{0.0f};
    float mvz{0.0f};

    // Target position.
    float tx{0.0f};
    float ty{0.0f};
    float tz{0.0f};

    // Commanded acceleration (optional).
    float ax{0.0f};
    float ay{0.0f};
    float az{0.0f};
};

/**
 * @brief Simple single-producer/single-consumer telemetry queue.
 *
 * Thread-safety:
 *   - push() and drain() are both mutex-protected and may be called
 *     from different threads (e.g., sim thread vs render thread).
 *
 * Behavior:
 *   - push() appends a sample and enforces a maximum queue size (max_).
 *   - drain() moves up to maxN samples into an output vector and
 *     removes them from the queue.
 */
class TelemetryBus
{
public:
    /// Push a single sample into the queue (thread-safe).
    void push(const TelemetrySample& s)
    {
        std::lock_guard<std::mutex> lock(m_);
        q_.push_back(s);
        if (q_.size() > max_) {
            q_.pop_front(); // cap growth
        }
    }

    /**
     * @brief Move up to maxN samples into 'out' and remove them from the queue.
     *
     * @param out  Destination vector (samples are appended).
     * @param maxN Maximum number of samples to drain.
     *
     * @return Number of samples drained.
     */
    [[nodiscard]] std::size_t drain(std::vector<TelemetrySample>& out,
                                    std::size_t maxN = 8192)
    {
        std::lock_guard<std::mutex> lock(m_);

        const std::size_t n = std::min<std::size_t>(maxN, q_.size());
        out.reserve(out.size() + n);

        for (std::size_t i = 0; i < n; ++i) {
            out.push_back(q_.front());
            q_.pop_front();
        }

        return n;
    }

    /// Set the maximum number of samples retained in the queue.
    void set_max(std::size_t m) { max_ = m; }

private:
    std::mutex                   m_;
    std::deque<TelemetrySample>  q_;
    std::size_t                  max_{200'000}; // 200k samples cap
};

} // namespace vis
