#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "ping/ping_sock.h"

namespace wiview {

/**
 * Traffic generator: continuously sends ICMP echo requests (pings) to the
 * gateway. The gateway's echo replies are received by the radio, and each
 * received packet produces a CSI sample via the esp_wifi CSI callback.
 *
 * ICMP is used instead of UDP-to-a-dead-port because gateways reliably
 * answer pings (a UDP packet to e.g. the discard port is usually dropped
 * silently, producing no reply and therefore no CSI).
 *
 * The ping runs asynchronously in its own FreeRTOS task, so update() is a
 * no-op kept only for call-site compatibility.
 */
class PacketGenerator {
public:
    /**
     * Start pinging target_ip (typically the gateway). interval_ms is the
     * spacing between echo requests (default 50ms → ~20 Hz).
     */
    static void begin(const IPAddress& target_ip, uint32_t interval_ms = 50);

    /** No-op: pings run in a background task. */
    static void update();

private:
    static IPAddress         s_target;
    static uint32_t          s_interval_ms;
    static esp_ping_handle_t s_ping;
};

} // namespace wiview
