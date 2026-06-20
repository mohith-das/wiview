#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace wiview::dsp {

class BreathingDetector {
public:
    BreathingDetector() { reset(); }

    void reset() {
        m_idx = 0; m_count = 0; m_ema_bpm = 0.0f; m_valid = false;
        for (size_t i = 0; i < WINDOW; i++) m_waveform[i] = 0.0f;
        for (size_t i = 0; i < SMOOTH_LEN; i++) m_smooth[i] = 0.0f;
        m_smooth_idx = 0;
    }

    void add(float amplitude) {
        m_waveform[m_idx] = amplitude;
        m_idx = (m_idx + 1) % WINDOW;
        if (m_count < WINDOW) m_count++;

        // DC removal: running mean
        float sum = 0.0f;
        for (size_t i = 0; i < m_count; i++) sum += m_waveform[i];
        float mean = sum / (float)m_count;
        float hp = amplitude - mean;

        // Smooth with short moving average
        m_smooth[m_smooth_idx] = hp;
        m_smooth_idx = (m_smooth_idx + 1) % SMOOTH_LEN;

        float lp = 0.0f;
        size_t sc = (m_count < (size_t)SMOOTH_LEN) ? m_count : (size_t)SMOOTH_LEN;
        for (size_t i = 0; i < sc; i++) lp += m_smooth[i];
        m_latest_waveform = lp / (float)sc;

        if (m_count >= WINDOW) {
            // Zero-crossing (pos->neg) over full window
            int crossings = 0;
            float prev_val = m_waveform[0] - mean;
            for (size_t i = 1; i < WINDOW; i++) {
                float curr_val = m_waveform[i] - mean;
                if (prev_val > 0.0f && curr_val <= 0.0f) crossings++;
                prev_val = curr_val;
            }
            float instant_bpm = (float)crossings * (60.0f / ((float)WINDOW / 20.0f));
            if (instant_bpm < 4.0f) instant_bpm = 4.0f;
            if (instant_bpm > 35.0f) instant_bpm = 35.0f;
            if (m_ema_bpm < 0.1f) m_ema_bpm = instant_bpm;
            else m_ema_bpm = 0.1f * instant_bpm + 0.9f * m_ema_bpm;
            m_valid = (m_ema_bpm >= 4.0f);
        }
    }

    float bpm() const { return m_ema_bpm; }
    float waveform() const { return m_latest_waveform; }
    bool valid() const { return m_valid && m_count >= WINDOW; }

private:
    static constexpr size_t WINDOW     = 128;
    static constexpr size_t SMOOTH_LEN = 4;

    float m_waveform[WINDOW];
    float m_smooth[SMOOTH_LEN];
    size_t m_idx, m_smooth_idx, m_count;
    float m_ema_bpm, m_latest_waveform;
    bool m_valid;
};

} // namespace wiview::dsp
