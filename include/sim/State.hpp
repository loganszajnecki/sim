#pragma once

#include <vector>
#include <cstddef>
#include <cassert>

namespace sim {

struct State {
    double t = 0.0; // simulation time
    std::vector<double> x; // generalized state vector

    State() = default;
    explicit State(size_t n) : x(n, 0.0) {}

    size_t size() const { return x.size(); }
};

// arithmetic helpers 
State operator+(const State& a, const State& b);
State operator-(const State& a, const State& b);
State scaled(const State& a, double s);
}