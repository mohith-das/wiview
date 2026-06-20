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

    /// Total packets sent since begin() (proxy for "actively streaming")
    uint32_t packetsSent() const { return m_sequence; }

    /// Current stream destination
    IPAddress target() const { return m_target; }

    /// Send a CSI raw data packet. Emits wiview's native format, or RuView's
    /// ADR-018 CSI frame (0xC5110001) when RuView mode is enabled.
    void sendCsiPacket(const int8_t* iq_data, uint16_t iq_len, uint32_t timestamp_ms);

    /// Send an inference JSON packet (wiview native format).
    void sendInference(const char* json);

    /// Send a RuView ADR-018 vitals packet (0xC5110002). Used in RuView mode.
    void sendVitals(bool presence, float motion, float bpm, uint32_t timestamp_ms);

    /// When true, sendCsiPacket emits RuView ADR-018 instead of wiview format,
    /// so the device can stream straight to a RuView sensing-server.
    void setRuViewMode(bool on) { m_ruview = on; }
    bool ruViewMode() const { return m_ruview; }

private:
    void sendCsiNative(const int8_t* iq_data, uint16_t iq_len, uint32_t timestamp_ms);
    void sendCsiRuView(const int8_t* iq_data, uint16_t iq_len);

    WiFiUDP m_udp;
    IPAddress m_target;
    uint16_t m_port = 5005;
    uint32_t m_sequence = 0;
    bool m_active = false;
    bool m_ruview = false;

    static constexpr uint32_t MAGIC = 0x77697669; // "wivi"
    static constexpr uint8_t  VERSION = 1;
    static constexpr uint8_t  TYPE_CSI = 0;
    static constexpr uint8_t  TYPE_INFERENCE = 1;

    // RuView ADR-018 packet magics (little-endian on the wire)
    static constexpr uint32_t RV_MAGIC_CSI    = 0xC5110001;
    static constexpr uint32_t RV_MAGIC_VITALS = 0xC5110002;
    static constexpr uint16_t RV_FREQ_MHZ     = 2412;  // stamped on CSI frames
    static constexpr int8_t   RV_RSSI         = -50;
    static constexpr int8_t   RV_NOISE        = -90;
    static constexpr uint8_t  RV_NODE_ID      = 1;
};

} // namespace wiview
