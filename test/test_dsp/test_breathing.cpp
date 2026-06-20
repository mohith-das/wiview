#include <unity.h>
#include "dsp/dsp_breathing.h"

using namespace wiview::dsp;
static constexpr float PI = 3.14159265358979f;

void test_breathing_initial_invalid() {
    BreathingDetector bd;
    TEST_ASSERT_FALSE(bd.valid());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, bd.bpm());
}

void test_breathing_sine_at_12bpm() {
    BreathingDetector bd;
    for (int i = 0; i < 512; i++) {
        float t = (float)i / 20.0f;
        float signal = 5.0f * sinf(2.0f * PI * 0.2f * t) + 50.0f;
        bd.add(signal);
    }
    TEST_ASSERT_TRUE(bd.valid());
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 12.0f, bd.bpm());
}

void test_breathing_constant_no_breathing() {
    BreathingDetector bd;
    for (int i = 0; i < 512; i++) {
        bd.add(50.0f);
    }
    TEST_ASSERT_TRUE(bd.valid());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 6.0f, bd.bpm());
}

void test_breathing_reset() {
    BreathingDetector bd;
    for (int i = 0; i < 512; i++) {
        float v = 50.0f + 5.0f * sinf((float)i * 0.0628f);
        bd.add(v);
    }
    TEST_ASSERT_TRUE(bd.valid());
    bd.reset();
    TEST_ASSERT_FALSE(bd.valid());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, bd.bpm());
}
