#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace wiview {

/**
 * Simple traffic generator: periodically sends a small UDP packet to
 * the gateway (or broadcast) to trigger AP responses. Each response
 * produces a CSI sample via the esp_wifi CSI callback.
 *
 * Called from loop() at ~10 ms intervals; internally rate-limits to
 * the configured interval.
 */
class PacketGenerator {
public:
    /**
     * Initialize the generator. target_ip is typically the gateway.
     * interval_ms is the desired packet spacing (default 50ms → ~20 Hz).
     */
    static void begin(const IPAddress& target_ip, uint32_t interval_ms = 50);

    /**
     * Call periodically from loop(). Sends a packet when the interval
     * has elapsed.
     */
    static void update();

private:
    static IPAddress s_target;
    static uint32_t  s_interval_ms;
    static uint32_t  s_last_send_ms;
    static int       s_sock;
    static uint16_t  s_seq;
};

} // namespace wiview
