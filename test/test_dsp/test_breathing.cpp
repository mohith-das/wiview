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

// Regression: broadband noise must NOT pin BPM at the 30 BPM ceiling.
// (Before the hysteresis/low-pass fix, noisy CSI amplitude produced many
//  spurious zero-crossings and the estimate slammed into the clamp.)
void test_breathing_noise_not_pinned_high() {
    BreathingDetector bd;
    uint32_t rng = 12345;
    for (int i = 0; i < 1024; i++) {
        rng = rng * 1103515245u + 12345u;          // cheap LCG
        float noise = ((float)((rng >> 16) & 0xFFFF) / 65535.0f - 0.5f) * 20.0f;
        bd.add(50.0f + noise);
    }
    TEST_ASSERT_TRUE(bd.bpm() < 28.0f);             // not stuck at the ceiling
}

// A 15 BPM breathing component buried in noise should still read plausibly.
void test_breathing_sine_in_noise() {
    BreathingDetector bd;
    uint32_t rng = 999;
    for (int i = 0; i < 1024; i++) {
        float t = (float)i / 20.0f;
        rng = rng * 1103515245u + 12345u;
        float noise = ((float)((rng >> 16) & 0xFFFF) / 65535.0f - 0.5f) * 3.0f;
        float signal = 6.0f * sinf(2.0f * PI * 0.25f * t) + 50.0f + noise;  // 0.25 Hz = 15 BPM
        bd.add(signal);
    }
    TEST_ASSERT_TRUE(bd.valid());
    TEST_ASSERT_FLOAT_WITHIN(7.0f, 15.0f, bd.bpm());
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
