#pragma once
#include <cstdint>
#include <cmath>

namespace wiview {

struct ImuSample {
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    uint32_t timestamp_ms;
};

struct ImuGateResult {
    bool device_still;
    float accel_magnitude;
    float gyro_magnitude;
};

inline ImuGateResult imu_evaluate(const ImuSample& s, float accel_thresh = 0.05f, float gyro_thresh = 3.0f) {
    ImuGateResult result;
    float raw_accel = std::sqrt(s.accel_x * s.accel_x + s.accel_y * s.accel_y + s.accel_z * s.accel_z);
    result.accel_magnitude = std::abs(raw_accel - 1.0f);
    result.gyro_magnitude = std::sqrt(s.gyro_x * s.gyro_x + s.gyro_y * s.gyro_y + s.gyro_z * s.gyro_z);
    result.device_still = (result.accel_magnitude <= accel_thresh) && (result.gyro_magnitude <= gyro_thresh);
    return result;
}

} // namespace wiview
