#include "wifi_manager.h"

namespace wiview {

bool WifiManager::connect(const char* ssid, const char* password) {
    Serial.printf("[WiFi] Connecting to %s...\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected. IP: %s, GW: %s\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.gatewayIP().toString().c_str());
        return true;
    }

    Serial.println("[WiFi] Connection failed.");
    return false;
}

IPAddress WifiManager::gateway() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.gatewayIP();
    }
    return IPAddress((uint32_t)0);
}

IPAddress WifiManager::localIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP();
    }
    return IPAddress((uint32_t)0);
}

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

} // namespace wiview
