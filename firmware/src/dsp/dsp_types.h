#pragma once
#include <cstdint>
#include <cstddef>

namespace wiview::dsp {

/// Maximum subcarriers in a single CSI sample (LLTF 20 MHz: 64, 40 MHz: 128)
static constexpr uint16_t MAX_SUBCARRIERS = 128;

/// Cleaned per-packet CSI vector
struct CsiVector {
    float amplitude[MAX_SUBCARRIERS];
    float phase[MAX_SUBCARRIERS];     // unwrapped + detrended, radians
    uint16_t num_subcarriers;
    uint32_t timestamp_ms;
    float mean_amplitude;
};

/// Output of the full preprocessing pipeline
struct PreprocessResult {
    CsiVector cleaned;
    float amplitude_variance;         // variance across subcarriers this packet
    bool valid;                       // false if input was corrupt
};

/// Welford running statistics for baseline calibration
struct BaselineStats {
    float mean_amplitude;
    float m2;                         // sum of squared differences (Welford)
    uint32_t sample_count;
    float variance;                   // m2 / count
    float stddev;                     // sqrt(variance)
};

/// Presence detection output
struct PresenceResult {
    bool present;
    float confidence;                 // 0.0 (empty) to 1.0 (certain)
    float z_score;                    // standard deviations from baseline
};

/// Motion level output
struct MotionResult {
    float motion_level;               // 0.0 (still) to 1.0 (active)
    float csi_power;                  // raw power in motion band
};

} // namespace wiview::dsp
