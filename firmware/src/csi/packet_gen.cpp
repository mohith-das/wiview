#include "packet_gen.h"
#include "lwip/ip_addr.h"

namespace wiview {

IPAddress         PacketGenerator::s_target;
uint32_t          PacketGenerator::s_interval_ms = 50;
esp_ping_handle_t PacketGenerator::s_ping = nullptr;

void PacketGenerator::begin(const IPAddress& target_ip, uint32_t interval_ms) {
    s_target      = target_ip;
    s_interval_ms = interval_ms;

    // Tear down any previous session (e.g. on reconnect).
    if (s_ping) {
        esp_ping_stop(s_ping);
        esp_ping_delete_session(s_ping);
        s_ping = nullptr;
    }

    // IPAddress -> lwip ip_addr_t (both store the v4 address in network order).
    ip_addr_t target = {};
    target.type = IPADDR_TYPE_V4;
    target.u_addr.ip4.addr = (uint32_t)target_ip;

    esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
    cfg.target_addr     = target;
    cfg.count           = ESP_PING_COUNT_INFINITE;  // ping forever
    cfg.interval_ms     = interval_ms;              // spacing between requests
    cfg.timeout_ms      = 1000;
    cfg.data_size       = 32;
    cfg.task_stack_size = 4096;

    // No callbacks needed: the echo replies are received by the radio, which
    // is all the CSI callback requires. We just need steady RX traffic.
    esp_ping_callbacks_t cbs = {};
    if (esp_ping_new_session(&cfg, &cbs, &s_ping) == ESP_OK && s_ping) {
        esp_ping_start(s_ping);
        Serial.printf("[PktGen] ICMP ping started, target=%s, interval=%ums\n",
                      s_target.toString().c_str(), s_interval_ms);
    } else {
        Serial.println("[PktGen] ERROR: esp_ping_new_session failed");
        s_ping = nullptr;
    }
}

void PacketGenerator::update() {
    // ICMP ping runs asynchronously in its own task; nothing to do here.
}

} // namespace wiview
