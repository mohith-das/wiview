#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <cstdint>

namespace wiview {

/// Streams CSI data + inferences over UDP to a host.
/// Call begin() once, then sendCsiPacket() and sendInference() from loop().
class UdpStreamer {
public:
    /// Initialize streaming to target_ip:port. Returns true if socket ready.
    bool begin(const IPAddress& target_ip, uint16_t port = 5005);

    /// Stop streaming and close socket.
    void stop();

    /// True if streaming is active
    bool isActive() const { return m_active; }

    /// Send a CSI raw data packet (binary header + I/Q bytes)
    void sendCsiPacket(const int8_t* iq_data, uint16_t iq_len, uint32_t timestamp_ms);

    /// Send an inference JSON packet
    void sendInference(const char* json);

private:
    WiFiUDP m_udp;
    IPAddress m_target;
    uint16_t m_port = 5005;
    uint32_t m_sequence = 0;
    bool m_active = false;

    static constexpr uint32_t MAGIC = 0x77697669; // "wivi"
    static constexpr uint8_t  VERSION = 1;
    static constexpr uint8_t  TYPE_CSI = 0;
    static constexpr uint8_t  TYPE_INFERENCE = 1;
};

} // namespace wiview
