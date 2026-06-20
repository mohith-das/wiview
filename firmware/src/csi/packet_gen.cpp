#include "packet_gen.h"
#include <lwip/sockets.h>

namespace wiview {

IPAddress PacketGenerator::s_target;
uint32_t  PacketGenerator::s_interval_ms = 50;
uint32_t  PacketGenerator::s_last_send_ms = 0;
int       PacketGenerator::s_sock = -1;
uint16_t  PacketGenerator::s_seq = 0;

void PacketGenerator::begin(const IPAddress& target_ip, uint32_t interval_ms) {
    s_target      = target_ip;
    s_interval_ms = interval_ms;
    s_last_send_ms = millis();

    s_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_sock >= 0) {
        // Set non-blocking
        int flags = fcntl(s_sock, F_GETFL, 0);
        fcntl(s_sock, F_SETFL, flags | O_NONBLOCK);
        Serial.printf("[PktGen] UDP socket created, target=%s, interval=%ums\n",
                      s_target.toString().c_str(), s_interval_ms);
    } else {
        Serial.println("[PktGen] ERROR: Failed to create UDP socket");
    }
}

void PacketGenerator::update() {
    if (s_sock < 0) return;

    uint32_t now = millis();
    if (now - s_last_send_ms < s_interval_ms) return;
    s_last_send_ms = now;

    // Send a small UDP packet to the target's port 9 (discard) or an arbitrary port
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(9);  // discard port — triggers ICMP or just ACKs
    addr.sin_addr.s_addr = (uint32_t)s_target;

    uint8_t payload[4];
    payload[0] = 'c';
    payload[1] = 's';
    payload[2] = 'i';
    payload[3] = (uint8_t)(s_seq++ & 0xFF);

    sendto(s_sock, payload, sizeof(payload), 0,
           (struct sockaddr*)&addr, sizeof(addr));
}

} // namespace wiview
