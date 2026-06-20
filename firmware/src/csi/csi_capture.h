#pragma once

#include <Arduino.h>

namespace wiview {

/**
 * Initialize ESP32 WiFi CSI capture.
 * Must be called AFTER WiFi is connected.
 * Configures LLTF mode, registers RX callback, enables CSI.
 * Returns true on success.
 */
bool csi_init();

/**
 * Total number of CSI packets received since csi_init().
 */
uint32_t csi_total_packets();

/**
 * Smoothed packet rate (packets per second), computed over a 2-second window.
 */
float csi_packet_rate();

/**
 * Average amplitude (sqrt(I^2+Q^2)) of the most recent CSI sample.
 * Range ~0-128 (8-bit signed I/Q magnitudes).
 */
float csi_latest_amplitude();

/**
 * Number of subcarriers in the most recent CSI sample.
 * Returns 0 if no samples received yet.
 */
uint16_t csi_subcarrier_count();

} // namespace wiview
