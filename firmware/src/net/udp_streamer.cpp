#include "udp_streamer.h"

namespace wiview {

bool UdpStreamer::begin(const IPAddress& target_ip, uint16_t port) {
    m_target = target_ip;
    m_port   = port;
    m_active = m_udp.begin(0); // any local port
    if (m_active) {
        Serial.printf("[UDP] Streaming to %s:%u\n", m_target.toString().c_str(), m_port);
    }
    return m_active;
}

void UdpStreamer::stop() {
    m_udp.stop();
    m_active = false;
    Serial.println("[UDP] Streaming stopped");
}

void UdpStreamer::sendCsiPacket(const int8_t* iq_data, uint16_t iq_len, uint32_t timestamp_ms) {
    if (!m_active || !iq_data || iq_len == 0) return;

    // Header: magic(4) + version(1) + type(1) + payload_len(2) + sequence(4) = 12 bytes
    // CSI payload: timestamp_ms(4) + num_subcarriers(2) + flags(2) + iq_data(iq_len)
    uint16_t payload_len = 8 + iq_len;
    uint8_t hdr[12];
    hdr[0] = (MAGIC >> 24) & 0xFF;
    hdr[1] = (MAGIC >> 16) & 0xFF;
    hdr[2] = (MAGIC >> 8) & 0xFF;
    hdr[3] = MAGIC & 0xFF;
    hdr[4] = VERSION;
    hdr[5] = TYPE_CSI;
    hdr[6] = (payload_len >> 8) & 0xFF;
    hdr[7] = payload_len & 0xFF;
    hdr[8]  = (m_sequence >> 24) & 0xFF;
    hdr[9]  = (m_sequence >> 16) & 0xFF;
    hdr[10] = (m_sequence >> 8) & 0xFF;
    hdr[11] = m_sequence & 0xFF;
    m_sequence++;

    m_udp.beginPacket(m_target, m_port);
    m_udp.write(hdr, 12);

    // CSI payload
    uint8_t ts_buf[4];
    ts_buf[0] = (timestamp_ms >> 24) & 0xFF;
    ts_buf[1] = (timestamp_ms >> 16) & 0xFF;
    ts_buf[2] = (timestamp_ms >> 8) & 0xFF;
    ts_buf[3] = timestamp_ms & 0xFF;
    m_udp.write(ts_buf, 4);

    uint16_t sc = iq_len / 2;
    m_udp.write((sc >> 8) & 0xFF);
    m_udp.write(sc & 0xFF);
    m_udp.write((uint8_t)0); // flags low
    m_udp.write((uint8_t)0); // flags high

    m_udp.write((const uint8_t*)iq_data, iq_len);
    m_udp.endPacket();
}

void UdpStreamer::sendInference(const char* json) {
    if (!m_active || !json) return;
    size_t json_len = strlen(json);
    uint16_t payload_len = json_len + 1; // +1 for null terminator

    uint8_t hdr[12];
    hdr[0] = (MAGIC >> 24) & 0xFF;
    hdr[1] = (MAGIC >> 16) & 0xFF;
    hdr[2] = (MAGIC >> 8) & 0xFF;
    hdr[3] = MAGIC & 0xFF;
    hdr[4] = VERSION;
    hdr[5] = TYPE_INFERENCE;
    hdr[6] = (payload_len >> 8) & 0xFF;
    hdr[7] = payload_len & 0xFF;
    hdr[8]  = (m_sequence >> 24) & 0xFF;
    hdr[9]  = (m_sequence >> 16) & 0xFF;
    hdr[10] = (m_sequence >> 8) & 0xFF;
    hdr[11] = m_sequence & 0xFF;
    m_sequence++;

    m_udp.beginPacket(m_target, m_port);
    m_udp.write(hdr, 12);
    m_udp.write((const uint8_t*)json, json_len);
    m_udp.endPacket();
}

} // namespace wiview
