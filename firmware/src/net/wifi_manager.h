#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace wiview {

class WifiManager {
public:
    /**
     * Connect to an AP in STA mode. Blocks until connected or timeout (15s).
     * Returns true on success. Uses hardcoded credentials for Phase 1.
     */
    static bool connect(const char* ssid = "wiview-test",
                        const char* password = "test1234");

    /** Get gateway IP after connection. Returns INADDR_NONE if not connected. */
    static IPAddress gateway();

    /** Get local IP. Returns INADDR_NONE if not connected. */
    static IPAddress localIP();

    /** Check if currently connected to an AP. */
    static bool isConnected();
};

} // namespace wiview
