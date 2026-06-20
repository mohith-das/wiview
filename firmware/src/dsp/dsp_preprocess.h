#pragma once
#include "dsp_types.h"
#include <cstdint>
#include <cmath>

namespace wiview::dsp {

inline uint16_t extract_amplitude(const int8_t* iq_data, uint16_t iq_len, float* amp_out) {
    uint16_t pairs = iq_len / 2;
    if (pairs > MAX_SUBCARRIERS) pairs = MAX_SUBCARRIERS;
    for (uint16_t i = 0; i < pairs; i++) {
        float iv = (float)iq_data[i * 2];
        float qv = (float)iq_data[i * 2 + 1];
        amp_out[i] = std::sqrt(iv * iv + qv * qv);
    }
    return pairs;
}

inline uint16_t extract_phase(const int8_t* iq_data, uint16_t iq_len, float* phase_out) {
    uint16_t pairs = iq_len / 2;
    if (pairs > MAX_SUBCARRIERS) pairs = MAX_SUBCARRIERS;
    for (uint16_t i = 0; i < pairs; i++) {
        float iv = (float)iq_data[i * 2];
        float qv = (float)iq_data[i * 2 + 1];
        phase_out[i] = std::atan2(qv, iv);
    }
    return pairs;
}

inline void unwrap_phase(float* phase, uint16_t count) {
    if (count < 2) return;
    float cumulative = 0.0f;
    float prev = phase[0];
    for (uint16_t i = 1; i < count; i++) {
        float diff = phase[i] - prev;
        if (diff > M_PI)       cumulative -= 2.0f * M_PI;
        else if (diff < -M_PI) cumulative += 2.0f * M_PI;
        prev = phase[i];
        phase[i] += cumulative;
    }
}

inline void detrend_phase(float* phase, uint16_t count) {
    if (count < 2) return;
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (uint16_t i = 0; i < count; i++) {
        float x = (float)i;
        float y = phase[i];
        sum_x  += x; sum_y += y; sum_xy += x * y; sum_x2 += x * x;
    }
    float n = (float)count;
    float denom = n * sum_x2 - sum_x * sum_x;
    if (denom == 0.0f) return;
    float a = (n * sum_xy - sum_x * sum_y) / denom;
    float b = (sum_y - a * sum_x) / n;
    for (uint16_t i = 0; i < count; i++) phase[i] -= (a * (float)i + b);
}

namespace detail {
inline bool is_pilot_or_null(uint16_t idx, uint16_t total) {
    if (total <= 64) {
        if (idx < 6 || idx == 32 || idx > 58) return true;
        if (idx == 7 || idx == 21 || idx == 43 || idx == 57) return true;
        return false;
    }
    if (idx < 6 || idx == 64 || idx > 121) return true;
    return false;
}
} // namespace detail

inline PreprocessResult preprocess(const int8_t* iq_data, uint16_t iq_len, uint32_t timestamp_ms) {
    PreprocessResult result = {};
    result.valid = false;
    if (iq_len < 4) return result;

    float raw_amp[MAX_SUBCARRIERS];
    float raw_phase[MAX_SUBCARRIERS];
    uint16_t num = extract_amplitude(iq_data, iq_len, raw_amp);
    extract_phase(iq_data, iq_len, raw_phase);
    unwrap_phase(raw_phase, num);
    detrend_phase(raw_phase, num);

    uint16_t out_idx = 0;
    float sum_amp = 0;
    for (uint16_t i = 0; i < num; i++) {
        if (detail::is_pilot_or_null(i, num)) continue;
        result.cleaned.amplitude[out_idx] = raw_amp[i];
        result.cleaned.phase[out_idx]     = raw_phase[i];
        sum_amp += raw_amp[i];
        out_idx++;
    }
    if (out_idx == 0) return result;
    result.cleaned.num_subcarriers = out_idx;
    result.cleaned.timestamp_ms    = timestamp_ms;
    result.cleaned.mean_amplitude  = sum_amp / (float)out_idx;

    float var_sum = 0;
    for (uint16_t i = 0; i < out_idx; i++) {
        float diff = result.cleaned.amplitude[i] - result.cleaned.mean_amplitude;
        var_sum += diff * diff;
    }
    result.amplitude_variance = var_sum / (float)out_idx;
    result.valid = true;
    return result;
}

} // namespace wiview::dsp
