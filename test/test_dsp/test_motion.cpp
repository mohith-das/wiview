#include <unity.h>
#include "dsp/dsp_motion.h"

using namespace wiview::dsp;

void test_motion_initial_zero() {
    MotionDetector md;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, md.motion_level());
}

void test_motion_with_constant_signal() {
    MotionDetector md;
    for (int i = 0; i < 32; i++) md.add(50.0f, i * 50);
    // Constant signal -> low variance -> low motion
    TEST_ASSERT_TRUE(md.motion_level() < 0.3f);
}

void test_motion_with_varying_signal() {
    MotionDetector md;
    // Alternating high/low -> high variance
    for (int i = 0; i < 32; i++) {
        md.add((i % 2 == 0) ? 20.0f : 80.0f, i * 50);
    }
    TEST_ASSERT_TRUE(md.motion_level() > 0.1f);
}

void test_motion_reset() {
    MotionDetector md;
    for (int i = 0; i < 32; i++) md.add(50.0f, i * 50);
    md.reset();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, md.motion_level());
}
