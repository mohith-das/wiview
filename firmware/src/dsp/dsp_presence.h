#pragma once
#include "dsp_types.h"
#include "dsp_calibration.h"
#include <cmath>
#include <algorithm>

namespace wiview::dsp {

inline PresenceResult detect_presence(float current_var, const Calibrator& baseline, float z_threshold = 3.0f) {
    PresenceResult result = {};
    result.present = false;
    result.confidence = 0.0f;
    result.z_score = 0.0f;

    if (!baseline.is_ready() || baseline.stddev() < 0.001f) return result;

    float baseline_var = baseline.stats().variance;
    float baseline_std = baseline.stats().stddev;
    result.z_score = (current_var - baseline_var) / baseline_std;

    if (result.z_score > 0.0f) {
        result.present = result.z_score >= z_threshold;
        result.confidence = std::min(1.0f, result.z_score / (2.0f * z_threshold));
    }
    return result;
}

} // namespace wiview::dsp
