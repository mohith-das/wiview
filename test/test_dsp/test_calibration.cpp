#include <unity.h>
#include "dsp/dsp_calibration.h"
#include <cmath>

using namespace wiview::dsp;

void test_calibrator_initial_state() {
    Calibrator cal;
    TEST_ASSERT_EQUAL(0, cal.count());
    TEST_ASSERT_FALSE(cal.is_ready());
}

void test_calibrator_converges_to_mean() {
    Calibrator cal;
    // Feed 100 samples with mean 50.0, stddev 10.0
    for (int i = 0; i < 100; i++) {
        float val = 50.0f + (float)((i % 21) - 10);  // range 40-60
        cal.add(val);
    }
    TEST_ASSERT_TRUE(cal.is_ready());
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 50.0f, cal.mean());
    TEST_ASSERT_TRUE(cal.stddev() > 0.0f);
}

void test_calibrator_reset() {
    Calibrator cal;
    cal.add(10.0f);
    cal.add(20.0f);
    cal.reset();
    TEST_ASSERT_EQUAL(0, cal.count());
    TEST_ASSERT_FALSE(cal.is_ready());
}
