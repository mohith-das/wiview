#pragma once
#include <cstdint>

/// Synthetic CSI I/Q data for native unit tests.
/// All data is algorithmically generated — no real captures.

namespace wiview::test {

/// Simple 4-subcarrier I/Q: [I0,Q0, I1,Q1, I2,Q2, I3,Q3]
/// Q components are zero → amplitude = |I|, phase = 0 or pi
static const int8_t fixture_simple_4sc[] = {
    10, 0,    // sc0: amp=10, phase=0
    -5, 0,    // sc1: amp=5,  phase=pi
    20, 0,    // sc2: amp=20, phase=0
    0, 10,    // sc3: amp=10, phase=pi/2
};

/// 8-subcarrier with linearly increasing phase (for detrend testing)
static const int8_t fixture_detrend_8sc[] = {
    10, 0,     // sc0: amp=10, phase=0
    9, 3,      // sc1: amp~9.5, phase~0.32
    8, 5,      // sc2: amp~9.4, phase~0.56
    7, 7,      // sc3: amp~9.9, phase~0.79
    5, 8,      // sc4: amp~9.4, phase~1.01
    3, 9,      // sc5: amp~9.5, phase~1.25
    0, 10,     // sc6: amp=10, phase~1.57
   -3, 9,      // sc7: amp~9.5, phase~1.89
};

/// 64-subcarrier simulated I/Q — alternating I=10,Q=0 and I=7,Q=7
static const int8_t fixture_64sc_alternating[128] = {
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
    10,0, 7,7, 10,0, 7,7, 10,0, 7,7, 10,0, 7,7,
};

} // namespace wiview::test
