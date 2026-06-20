#include <unity.h>

// preprocess
void test_extract_amplitude_simple();
void test_extract_phase_simple();
void test_unwrap_no_wraps();
void test_unwrap_with_jump();
void test_detrend_removes_slope();
void test_preprocess_returns_valid();
void test_preprocess_empty_input();

// calibration
void test_calibrator_initial_state();
void test_calibrator_converges_to_mean();
void test_calibrator_reset();

// presence
void test_no_presence_at_baseline();
void test_presence_with_high_variance();
void test_calibrator_not_ready_gives_no_presence();

// motion
void test_motion_initial_zero();
void test_motion_with_constant_signal();
void test_motion_with_varying_signal();
void test_motion_reset();

// breathing
void test_breathing_initial_invalid();
void test_breathing_sine_at_12bpm();
void test_breathing_constant_no_breathing();
void test_breathing_noise_not_pinned_high();
void test_breathing_sine_in_noise();
void test_breathing_reset();

// imu gate
void test_device_still();
void test_device_bumped();
void test_device_tilted();
void test_custom_thresholds();

int main() {
    UNITY_BEGIN();
    // preprocess
    RUN_TEST(test_extract_amplitude_simple);
    RUN_TEST(test_extract_phase_simple);
    RUN_TEST(test_unwrap_no_wraps);
    RUN_TEST(test_unwrap_with_jump);
    RUN_TEST(test_detrend_removes_slope);
    RUN_TEST(test_preprocess_returns_valid);
    RUN_TEST(test_preprocess_empty_input);
    // calibration
    RUN_TEST(test_calibrator_initial_state);
    RUN_TEST(test_calibrator_converges_to_mean);
    RUN_TEST(test_calibrator_reset);
    // presence
    RUN_TEST(test_no_presence_at_baseline);
    RUN_TEST(test_presence_with_high_variance);
    RUN_TEST(test_calibrator_not_ready_gives_no_presence);
    // motion
    RUN_TEST(test_motion_initial_zero);
    RUN_TEST(test_motion_with_constant_signal);
    RUN_TEST(test_motion_with_varying_signal);
    RUN_TEST(test_motion_reset);
    // breathing
    RUN_TEST(test_breathing_initial_invalid);
    RUN_TEST(test_breathing_sine_at_12bpm);
    RUN_TEST(test_breathing_constant_no_breathing);
    RUN_TEST(test_breathing_noise_not_pinned_high);
    RUN_TEST(test_breathing_sine_in_noise);
    RUN_TEST(test_breathing_reset);
    // imu gate
    RUN_TEST(test_device_still);
    RUN_TEST(test_device_bumped);
    RUN_TEST(test_device_tilted);
    RUN_TEST(test_custom_thresholds);
    return UNITY_END();
}
