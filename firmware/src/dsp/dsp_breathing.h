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
            // Window standard deviation sets a noise-aware hysteresis band, so
            // broadband CSI noise doesn't produce spurious zero-crossings.
            float var = 0.0f;
            for (size_t i = 0; i < WINDOW; i++) {
                float d = m_waveform[i] - mean;
                var += d * d;
            }
            float stddev = std::sqrt(var / (float)WINDOW);
            float thr = HYST_FRAC * stddev;  // must rise above +thr to "arm" a crossing

            // Walk the low-pass-filtered, DC-removed signal in chronological
            // order and record the sample index of each downward (pos->neg)
            // crossing. The low-pass rejects content above the breathing band;
            // hysteresis rejects jitter near zero.
            float ma_ring[LP] = {0.0f};
            float ma_sum = 0.0f;
            size_t ma_pos = 0, ma_filled = 0;
            int  cross_pos[MAX_CROSS];
            int  ncross = 0;
            bool armed = false, have_prev = false;
            float prev_lp = 0.0f;
            for (size_t k = 0; k < WINDOW; k++) {
                size_t idx = (m_idx + k) % WINDOW;   // oldest -> newest
                float x = m_waveform[idx] - mean;
                ma_sum -= ma_ring[ma_pos];
                ma_ring[ma_pos] = x;
                ma_sum += x;
                ma_pos = (ma_pos + 1) % LP;
                if (ma_filled < LP) ma_filled++;
                float lp = ma_sum / (float)ma_filled;

                if (lp > thr) armed = true;
                if (have_prev && armed && prev_lp > 0.0f && lp <= 0.0f) {
                    if (ncross < MAX_CROSS) cross_pos[ncross] = (int)k;
                    ncross++;
                    armed = false;
                }
                prev_lp = lp;
                have_prev = true;
            }
            if (ncross > MAX_CROSS) ncross = MAX_CROSS;

            // Estimate rate from the *period* between crossings rather than a
            // raw crossing count — a continuous value, so it doesn't jump in
            // coarse steps. The median of the intervals rejects a stray crossing.
            float instant_bpm;
            if (ncross >= 2) {
                int iv[MAX_CROSS];
                int n_iv = 0;
                for (int i = 1; i < ncross; i++) iv[n_iv++] = cross_pos[i] - cross_pos[i - 1];
                std::sort(iv, iv + n_iv);
                float median_iv = (n_iv & 1) ? (float)iv[n_iv / 2]
                                             : 0.5f * (iv[n_iv / 2 - 1] + iv[n_iv / 2]);
                if (median_iv < 1.0f) median_iv = 1.0f;
                instant_bpm = 60.0f * SAMPLE_HZ / median_iv;
            } else {
                // Too few crossings to measure a period (flat / no breathing).
                instant_bpm = (float)ncross * (60.0f * SAMPLE_HZ / (float)WINDOW);
            }

            if (instant_bpm < MIN_BPM) instant_bpm = MIN_BPM;
            if (instant_bpm > MAX_BPM) instant_bpm = MAX_BPM;
            if (m_ema_bpm < 0.1f) m_ema_bpm = instant_bpm;
            else m_ema_bpm = EMA_ALPHA * instant_bpm + (1.0f - EMA_ALPHA) * m_ema_bpm;
            m_valid = (m_ema_bpm >= MIN_BPM);
        }
    }

    float bpm() const { return m_ema_bpm; }
    float waveform() const { return m_latest_waveform; }
    bool valid() const { return m_valid && m_count >= WINDOW; }

private:
    // ~12.8 s at SAMPLE_HZ — long enough to capture several breaths so the
    // period (interval) estimate has multiple samples to median over.
    static constexpr size_t WINDOW     = 256;
    static constexpr size_t SMOOTH_LEN = 4;      // short MA for the UI waveform
    // Moving-average low-pass for the crossing detector. At 20 Hz, length 20
    // puts the first null near 1 Hz, passing the 0.1-0.5 Hz breathing band
    // while smoothing broadband CSI noise below the hysteresis band.
    static constexpr size_t LP         = 20;
    static constexpr size_t MAX_CROSS  = 24;     // max crossings tracked per window
    static constexpr float  SAMPLE_HZ  = 20.0f;  // expected feed rate (Hz)
    static constexpr float  HYST_FRAC  = 0.35f;  // hysteresis as fraction of window std
    static constexpr float  EMA_ALPHA  = 0.1f;   // temporal smoothing of the estimate
    static constexpr float  MIN_BPM    = 4.0f;
    static constexpr float  MAX_BPM    = 30.0f;  // plausible human breathing ceiling

    float m_waveform[WINDOW];
    float m_smooth[SMOOTH_LEN];
    size_t m_idx, m_smooth_idx, m_count;
    float m_ema_bpm, m_latest_waveform;
    bool m_valid;
};

} // namespace wiview::dsp
