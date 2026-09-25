#pragma once

#include <Adp.h>

#include <deque>
#include <cstddef>

namespace adp {

class GraphHistory
{
public:

    std::deque<double> values;
    static constexpr size_t SAMPLE_RATE = 120; // Samples per second
    static constexpr size_t MAX_SAMPLES = 3 * SAMPLE_RATE; // 3 seconds of data

    void AddValue(double value) {
        values.push_back(value);
        if (values.size() > MAX_SAMPLES) {
            values.pop_front();
        }
    }

    void Clear() {
        values.clear();
    }
};

}; // namespace adp.
