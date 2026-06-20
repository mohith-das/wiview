#include <unity.h>
#include "dsp/dsp_preprocess.h"
#include "test_fixtures.h"
#include <cmath>

using namespace wiview::dsp;
using namespace wiview::test;

void test_extract_amplitude_simple() {
    float amp[4];
    uint16_t n = extract_amplitude(fixture_simple_4sc, 8, amp);
    TEST_ASSERT_EQUAL(4, n);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, amp[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5.0f,  amp[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f, amp[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, amp[3]);
}

void test_extract_phase_simple() {
    float phase[4];
    extract_phase(fixture_simple_4sc, 8, phase);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f,      phase[0]);   // I+ Q0 -> 0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)M_PI, phase[1]); // I- Q0 -> pi
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f,      phase[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)(M_PI/2), phase[3]); // I0 Q+ -> pi/2
}

void test_unwrap_no_wraps() {
    float phase[] = {0.5f, 1.0f, 1.5f};
    unwrap_phase(phase, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, phase[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, phase[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.5f, phase[2]);
}

void test_unwrap_with_jump() {
    // Simulate phase wrapping: -3.0, 3.0, -3.0 (crosses +pi/-pi boundary)
    float phase[] = {-3.0f, 3.0f, -3.0f};
    unwrap_phase(phase, 3);
    // After unwrap: -3.0, ~3.0-2*pi, ~ -3.0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -3.0f, phase[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f,  3.0f - 2.0f * M_PI, phase[1]);
    // Third should be close to -3.0 (no additional wrap since phase[1] is now negative)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -3.0f, phase[2]);
}

void test_detrend_removes_slope() {
    float phase[] = {0.1f, 1.1f, 2.1f, 3.1f};  // slope = 1.0, offset = 0.1
    detrend_phase(phase, 4);
    // After detrend, all should be near 0
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, phase[i]);
    }
}

void test_preprocess_returns_valid() {
    PreprocessResult r = preprocess(fixture_64sc_alternating, 128, 0);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_TRUE(r.cleaned.num_subcarriers > 0);
    // 64 carriers, minus pilot/null (~11) = ~53 usable
    TEST_ASSERT_TRUE(r.cleaned.num_subcarriers > 40);
    TEST_ASSERT_TRUE(r.cleaned.num_subcarriers <= 64);
}

void test_preprocess_empty_input() {
    PreprocessResult r = preprocess(nullptr, 0, 0);
    TEST_ASSERT_FALSE(r.valid);
}
