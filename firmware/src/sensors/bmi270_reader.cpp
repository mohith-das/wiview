#include "bmi270_reader.h"

#if !defined(PLATFORMIO_NATIVE)
#include <M5Unified.h>

namespace wiview {

ImuSample bmi270_read() {
    ImuSample s = {};
    s.timestamp_ms = millis();

    if (M5.Imu.isEnabled()) {
        M5.Imu.getAccel(&s.accel_x, &s.accel_y, &s.accel_z);
        M5.Imu.getGyro(&s.gyro_x, &s.gyro_y, &s.gyro_z);
    }
    return s;
}

} // namespace wiview

#endif // !PLATFORMIO_NATIVE
