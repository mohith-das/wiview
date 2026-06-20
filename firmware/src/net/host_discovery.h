#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace wiview {

/**
 * Zero-config host discovery.
 *
 * The wiview host companion periodically broadcasts an "announce" beacon on the
 * LAN. This listener receives those beacons and reports the host's address +
 * the port it wants CSI streamed to, so the user never has to type an IP.
 *
 * Beacon datagram (big-endian), sent by the host to UDP DISCOVERY_PORT:
 *   magic   u32  0x77697669 ("wivi")
 *   version u8   1
 *   type    u8   2 (announce)
 *   port    u16  the port the host is listening on for CSI (e.g. 5005)
 */
class HostDiscovery {
public:
    static constexpr uint16_t DISCOVERY_PORT = 5008;

    /// Bind the discovery socket. Call after WiFi connects. Returns true on success.
    static bool begin();

    /// Non-blocking poll. If a valid beacon arrived, fills host/port and returns
    /// true. Returns false when there is nothing new.
    static bool poll(IPAddress& out_host, uint16_t& out_port);

private:
    static int s_sock;
};

} // namespace wiview
