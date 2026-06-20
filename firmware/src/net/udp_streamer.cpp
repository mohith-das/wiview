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
    if (m_ruview) sendCsiRuView(iq_data, iq_len);
    else          sendCsiNative(iq_data, iq_len, timestamp_ms);
}

void UdpStreamer::sendCsiNative(const int8_t* iq_data, uint16_t iq_len, uint32_t timestamp_ms) {
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

// RuView ADR-018 CSI frame (little-endian, 20-byte header + int8 I/Q pairs).
void UdpStreamer::sendCsiRuView(const int8_t* iq_data, uint16_t iq_len) {
    uint8_t  hdr[20];
    uint16_t n_sub = iq_len / 2;

    hdr[0] = RV_MAGIC_CSI & 0xFF;
    hdr[1] = (RV_MAGIC_CSI >> 8) & 0xFF;
    hdr[2] = (RV_MAGIC_CSI >> 16) & 0xFF;
    hdr[3] = (RV_MAGIC_CSI >> 24) & 0xFF;
    hdr[4] = RV_NODE_ID;
    hdr[5] = 1;                              // n_antennas
    hdr[6] = n_sub & 0xFF;                    // n_subcarriers (LE u16)
    hdr[7] = (n_sub >> 8) & 0xFF;
    hdr[8]  = RV_FREQ_MHZ & 0xFF;             // freq_mhz (LE u32)
    hdr[9]  = (RV_FREQ_MHZ >> 8) & 0xFF;
    hdr[10] = 0;
    hdr[11] = 0;
    hdr[12] = m_sequence & 0xFF;              // seq (LE u32)
    hdr[13] = (m_sequence >> 8) & 0xFF;
    hdr[14] = (m_sequence >> 16) & 0xFF;
    hdr[15] = (m_sequence >> 24) & 0xFF;
    hdr[16] = (uint8_t)RV_RSSI;               // rssi (i8)
    hdr[17] = (uint8_t)RV_NOISE;              // noise_floor (i8)
    hdr[18] = 0;                              // ppdu_type
    hdr[19] = 0;                              // flags
    m_sequence++;

    m_udp.beginPacket(m_target, m_port);
    m_udp.write(hdr, 20);
    m_udp.write((const uint8_t*)iq_data, iq_len);
    m_udp.endPacket();
}

// RuView ADR-018 vitals packet (0xC5110002, 32 bytes, little-endian).
void UdpStreamer::sendVitals(bool presence, float motion, float bpm, uint32_t timestamp_ms) {
    if (!m_active) return;

    uint8_t pkt[32] = {};
    pkt[0] = RV_MAGIC_VITALS & 0xFF;
    pkt[1] = (RV_MAGIC_VITALS >> 8) & 0xFF;
    pkt[2] = (RV_MAGIC_VITALS >> 16) & 0xFF;
    pkt[3] = (RV_MAGIC_VITALS >> 24) & 0xFF;
    pkt[4] = RV_NODE_ID;
    pkt[5] = (presence ? 0x1 : 0) | (motion > 0.3f ? 0x4 : 0);  // flags
    uint16_t br = (uint16_t)(bpm > 0 ? bpm * 100.0f : 0);        // breathing*100
    pkt[6] = br & 0xFF;
    pkt[7] = (br >> 8) & 0xFF;
    // pkt[8..11] heartrate u32 = 0
    pkt[12] = (uint8_t)RV_RSSI;
    pkt[13] = presence ? 1 : 0;                                  // n_persons
    // pkt[14..15] pad
    memcpy(&pkt[16], &motion, 4);                                // motion_energy f32 (LE)
    float conf = presence ? 0.8f : 0.2f;
    memcpy(&pkt[20], &conf, 4);                                  // presence_score f32 (LE)
    memcpy(&pkt[24], &timestamp_ms, 4);                          // timestamp_ms u32 (LE)
    // pkt[28..31] reserved = 0

    m_udp.beginPacket(m_target, m_port);
    m_udp.write(pkt, 32);
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
