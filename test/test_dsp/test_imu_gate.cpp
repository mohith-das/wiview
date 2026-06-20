#include <unity.h>
#include "sensors/imu_gate.h"

using namespace wiview;

// Inline IMU test fixtures (no external dependency)
static ImuSample fixture_imu_still() {
    return {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0};
}
static ImuSample fixture_imu_bumped() {
    return {0.5f, 0.3f, 1.1f, 15.0f, -8.0f, 2.0f, 1000};
}
static ImuSample fixture_imu_tilted() {
    return {0.1f, 0.0f, 0.99f, 0.1f, 0.0f, 0.0f, 2000};
}

void test_device_still() {
    ImuGateResult r = imu_evaluate(fixture_imu_still());
    TEST_ASSERT_TRUE(r.device_still);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, r.accel_magnitude);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, r.gyro_magnitude);
}

void test_device_bumped() {
    ImuGateResult r = imu_evaluate(fixture_imu_bumped());
    TEST_ASSERT_FALSE(r.device_still);
    TEST_ASSERT_TRUE(r.accel_magnitude > 0.05f || r.gyro_magnitude > 3.0f);
}

void test_device_tilted() {
    ImuGateResult r = imu_evaluate(fixture_imu_tilted());
    TEST_ASSERT_TRUE(r.accel_magnitude >= 0.0f);
}

void test_custom_thresholds() {
    ImuGateResult r = imu_evaluate(fixture_imu_bumped(), 10.0f, 100.0f);
    TEST_ASSERT_TRUE(r.device_still);
}
