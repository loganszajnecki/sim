#pragma once
#include <deque>
#include <mutex>
#include <vector>

namespace vis {

// one sample per integrator step (units: meters, m/s; Z-up)
struct TelemetrySample {
    double t{0.0};
    float  mx{0}, my{0}, mz{0};     // missile position
    float  mvx{0}, mvy{0}, mvz{0};  // missile velocity
    float  tx{0}, ty{0}, tz{0};     // target position
    float  ax{0}, ay{0}, az{0};     // commanded accel (optional)
};

// simple single-producer/single-consumer queue (mutex+deque for now)
class TelemetryBus
{
public:
    void push(const TelemetrySample& s) {
        std::lock_guard<std::mutex> lock(m_);
        q_.push_back(s);
        if (q_.size() > max_) q_.pop_front(); // cap growth
    }

    // Move up to maxN samples into 'out' and clear them from the queue
    size_t drain(std::vector<TelemetrySample>& out, size_t maxN = 8192) {
        std::lock_guard<std::mutex> lock(m_);
        const size_t n = std::min(maxN, q_.size());
        out.reserve(out.size() + n);
        for (size_t i = 0; i < n; ++i) {
            out.push_back(q_.front());
            q_.pop_front();
        }
        return n;
    }

    void set_max(size_t m) { max_ = m; }

private:
    std::mutex m_;
    std::deque<TelemetrySample> q_;
    size_t max_ = 200000; // 200k samples cap
};

} // namespace vis
