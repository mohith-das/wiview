#include "host_discovery.h"
#include <lwip/sockets.h>

namespace wiview {

int HostDiscovery::s_sock = -1;

static constexpr uint32_t BEACON_MAGIC   = 0x77697669;  // "wivi"
static constexpr uint8_t  BEACON_VERSION = 1;
static constexpr uint8_t  BEACON_ANNOUNCE = 2;

bool HostDiscovery::begin() {
    if (s_sock >= 0) return true;

    s_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_sock < 0) {
        Serial.println("[Discovery] socket() failed");
        return false;
    }

    int yes = 1;
    setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Serial.printf("[Discovery] bind(%u) failed\n", DISCOVERY_PORT);
        close(s_sock);
        s_sock = -1;
        return false;
    }

    int flags = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, flags | O_NONBLOCK);
    Serial.printf("[Discovery] listening for host beacons on UDP %u\n", DISCOVERY_PORT);
    return true;
}

bool HostDiscovery::poll(IPAddress& out_host, uint16_t& out_port) {
    if (s_sock < 0) return false;

    uint8_t buf[8];
    struct sockaddr_in from = {};
    socklen_t from_len = sizeof(from);
    int n = recvfrom(s_sock, buf, sizeof(buf), 0,
                     (struct sockaddr*)&from, &from_len);
    if (n < 8) return false;  // too short / nothing waiting

    uint32_t magic = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
                     ((uint32_t)buf[2] << 8) | buf[3];
    if (magic != BEACON_MAGIC || buf[4] != BEACON_VERSION || buf[5] != BEACON_ANNOUNCE) {
        return false;
    }

    uint16_t port = ((uint16_t)buf[6] << 8) | buf[7];
    if (port == 0) return false;

    out_host = IPAddress((uint32_t)from.sin_addr.s_addr);  // network byte order
    out_port = port;
    return true;
}

} // namespace wiview
