#pragma once
#include <fstream>
#include <string>

namespace app {

class TelemetryCsvWriter {
public:
    explicit TelemetryCsvWriter(const std::string& path)
        : out_(path, std::ios::out)
    {
        out_.setf(std::ios::unitbuf);

        if (out_) {
            out_ << "t,px,py,pz,vx,vy,vz,tx,ty,tz\n";
        }
    }

    bool good() const { return out_.good(); }

    template <class S>
    void write(double t, const S& m, const S& tgt) {
        out_ << t << ","
             << m.x[0] << "," << m.x[1] << "," << m.x[2] << ","
             << m.x[3] << "," << m.x[4] << "," << m.x[5] << ","
             << tgt.x[0] << "," << tgt.x[1] << "," << tgt.x[2] << std::endl;
    }

    void flush() { out_.flush(); }

private:
    std::ofstream out_;
};

} // namespace app