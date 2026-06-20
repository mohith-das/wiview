#pragma once
#include "imu_gate.h"
#include <cstdint>

namespace wiview {

/// Hardware-specific: reads BMI270 via M5.Imu and returns an ImuSample.
/// Only compiles on the cardputer target (includes M5Unified).
/// For native tests, use imu_evaluate() directly with synthetic data.
ImuSample bmi270_read();

} // namespace wiview
