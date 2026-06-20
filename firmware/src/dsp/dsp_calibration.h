#pragma once
#include "dsp_types.h"
#include <cstdint>
#include <cmath>

namespace wiview::dsp {

class Calibrator {
public:
    Calibrator() { reset(); }

    void reset() {
        m_stats.mean_amplitude = 0.0f;
        m_stats.m2 = 0.0f;
        m_stats.sample_count = 0;
        m_stats.variance = 0.0f;
        m_stats.stddev  = 0.0f;
    }

    void add(float x) {
        m_stats.sample_count++;
        float delta = x - m_stats.mean_amplitude;
        m_stats.mean_amplitude += delta / (float)m_stats.sample_count;
        float delta2 = x - m_stats.mean_amplitude;
        m_stats.m2 += delta * delta2;
        if (m_stats.sample_count >= 2) {
            m_stats.variance = m_stats.m2 / (float)m_stats.sample_count;
            m_stats.stddev  = std::sqrt(m_stats.variance);
        }
    }

    uint32_t count() const { return m_stats.sample_count; }
    bool is_ready() const { return m_stats.sample_count >= 100; }
    const BaselineStats& stats() const { return m_stats; }
    float mean() const { return m_stats.mean_amplitude; }
    float stddev() const { return m_stats.stddev; }

private:
    BaselineStats m_stats;
};

} // namespace wiview::dsp
