#pragma once
#include "dsp_types.h"
#include <cstdint>
#include <algorithm>

namespace wiview::dsp {

class MotionDetector {
public:
    MotionDetector() { reset(); }

    void reset() {
        m_idx = 0; m_count = 0; m_mean = 0.0f; m_variance = 0.0f;
        for (size_t i = 0; i < WINDOW; i++) { m_history[i] = 0.0f; m_timestamps[i] = 0; }
    }

    void add(float amp, uint32_t ts) {
        m_history[m_idx] = amp;
        m_timestamps[m_idx] = ts;
        m_idx = (m_idx + 1) % WINDOW;
        if (m_count < WINDOW) m_count++;

        float sum = 0.0f;
        for (size_t i = 0; i < m_count; i++) sum += m_history[i];
        m_mean = sum / (float)m_count;

        float var_sum = 0.0f;
        for (size_t i = 0; i < m_count; i++) {
            float diff = m_history[i] - m_mean;
            var_sum += diff * diff;
        }
        m_variance = m_count > 1 ? var_sum / (float)m_count : 0.0f;
    }

    float motion_level() const {
        if (m_count < 4) return 0.0f;
        float level = m_variance / 100.0f;
        if (level > 1.0f) level = 1.0f;
        return level;
    }

    float raw_variance() const { return m_variance; }

private:
    static constexpr size_t WINDOW = 32;
    float m_history[WINDOW];
    uint32_t m_timestamps[WINDOW];
    size_t m_idx;
    size_t m_count;
    float m_mean;
    float m_variance;
};

} // namespace wiview::dsp
