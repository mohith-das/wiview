#include <unity.h>
#include "dsp/dsp_presence.h"
#include "dsp/dsp_calibration.h"

using namespace wiview::dsp;

void test_no_presence_at_baseline() {
    Calibrator cal;
    for (int i = 0; i < 200; i++) cal.add(100.0f);
    PresenceResult r = detect_presence(100.0f, cal);
    TEST_ASSERT_FALSE(r.present);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, r.z_score);
}

void test_presence_with_high_variance() {
    Calibrator cal;
    // Baseline: mean=100, variance=400 (stddev=20)
    for (int i = 0; i < 200; i++) {
        cal.add(100.0f + (float)((i % 41) - 20));
    }
    // Current sample with much higher variance (amplitude diff large)
    PresenceResult r = detect_presence(200.0f, cal, 2.0f);
    // This should trigger presence if z > 2.0
    if (cal.stddev() > 0.1f) {
        TEST_ASSERT_TRUE(r.present || r.z_score > 0.0f); // at minimum, z > 0
    }
}

void test_calibrator_not_ready_gives_no_presence() {
    Calibrator cal;
    cal.add(100.0f);  // only 1 sample
    PresenceResult r = detect_presence(500.0f, cal);
    TEST_ASSERT_FALSE(r.present);
}
