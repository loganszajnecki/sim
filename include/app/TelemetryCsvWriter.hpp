#pragma once
#include <fstream>
#include <string>

namespace app {

class TelemetryCsvWriter {
public:
    explicit TelemetryCsvWriter(const std::string& path, bool line_flush = true)
        : out_(path, std::ios::out), line_flush_(line_flush) {
        if (out_) {
            if (line_flush_) out_.setf(std::ios::unitbuf);
            out_ << "t,px,py,pz,vx,vy,vz,tx,ty,tz\n";
        }
    }
    bool good() const { return out_.good(); }

    template <class S>
    void write(double t, const S& m, const S& tgt) {
        out_ << t << ","
             << m.x[0] << "," << m.x[1] << "," << m.x[2] << ","
             << m.x[3] << "," << m.x[4] << "," << m.x[5] << ","
             << tgt.x[0] << "," << tgt.x[1] << "," << tgt.x[2];
        if (line_flush_) out_ << std::endl;
        else out_ << '\n';
    }
    void flush() { out_.flush(); }

private:
    std::ofstream out_;
    bool line_flush_ = true;
};

} // namespace app