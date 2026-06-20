#pragma once
#include <cstdint>

namespace wiview {

/// All sensor data fed to UI screens each update cycle
struct SensorData {
    // CSI health
    float csi_rate_hz = 0.0f;
    uint16_t subcarrier_count = 0;
    uint32_t total_packets = 0;
    float latest_amplitude = 0.0f;

    // Presence
    bool presence_detected = false;
    float presence_confidence = 0.0f;
    float presence_z_score = 0.0f;

    // Motion
    float motion_level = 0.0f;

    // IMU gate
    bool device_still = true;
    float accel_magnitude = 0.0f;
    float gyro_magnitude = 0.0f;

    // Calibration
    bool calibrated = false;
    uint32_t cal_samples = 0;

    // Breathing
    float breathing_bpm = 0.0f;
    float breathing_waveform = 0.0f;
    bool breathing_valid = false;

    // Waterfall data (latest amplitude per subcarrier, allocated separately)
    float* amp_per_sc = nullptr;
    uint16_t num_sc = 0;

    // UDP streaming state (toggled by the 's' key)
    bool streaming = false;
    uint32_t stream_target_ip = 0;  // destination IP when streaming (UDP: not a confirmed connection)
    uint32_t stream_packets = 0;    // packets sent this session (proxy for "actively streaming")
    bool ruview_mode = false;       // true = emit RuView ADR-018; false = wiview native format
    bool wifi_forget_armed = false; // 'w' pressed once; awaiting confirm to forget WiFi
};

} // namespace wiview
