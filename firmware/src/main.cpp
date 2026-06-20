/**
 * wiview — Phase 1: CSI capture + health on Cardputer-Adv
 *
 * Connects to WiFi, initializes ESP32 CSI capture, generates traffic
 * to the gateway, and displays live packets-per-second + amplitude bar
 * on the Cardputer screen.
 */
#include <M5Cardputer.h>
#include <M5Unified.h>
#include "net/wifi_manager.h"
#include "csi/csi_capture.h"
#include "csi/packet_gen.h"

using namespace wiview;

// ── Hardcoded WiFi credentials for Phase 1 ────────────────────────────────
// TODO: Replace with on-screen provisioning + NVS in Phase 4
static constexpr const char* WIFI_SSID     = "wiview-test";
static constexpr const char* WIFI_PASSWORD = "test1234";

static bool g_wifi_ok = false;
static bool g_csi_ok  = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=== wiview Phase 1 — CSI Capture ===\n");

    // Init Cardputer hardware
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    Serial.printf("[HW] Display: %dx%d\n",
                  M5Cardputer.Display.width(),
                  M5Cardputer.Display.height());

    M5Cardputer.Display.clear(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.println("wiview");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(4, 36);
    M5Cardputer.Display.println("Phase 1: CSI");

    // Connect WiFi
    g_wifi_ok = WifiManager::connect(WIFI_SSID, WIFI_PASSWORD);

    if (g_wifi_ok) {
        // Init CSI capture
        g_csi_ok = csi_init();

        // Start packet generator
        IPAddress gw = WifiManager::gateway();
        PacketGenerator::begin(gw, 50);  // 50 ms → ~20 Hz target
    }

    // Draw status bar
    M5Cardputer.Display.setCursor(4, 60);
    if (g_wifi_ok) {
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Cardputer.Display.printf("WiFi: %s", WIFI_SSID);
    } else {
        M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5Cardputer.Display.println("WiFi: FAILED");
    }

    M5Cardputer.Display.setCursor(4, 72);
    if (g_csi_ok) {
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Cardputer.Display.println("CSI:  waiting...");
    } else if (g_wifi_ok) {
        M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5Cardputer.Display.println("CSI:  FAILED");
    }
}

void loop() {
    M5Cardputer.update();

    // Generate traffic
    if (g_wifi_ok) {
        PacketGenerator::update();
    }

    // Update display every 200ms
    static uint32_t last_disp = 0;
    uint32_t now = millis();
    if (now - last_disp >= 200) {
        last_disp = now;

        // Clear metrics area
        M5Cardputer.Display.fillRect(0, 84, 240, 51, TFT_BLACK);

        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 88);

        if (g_csi_ok) {
            uint32_t total  = csi_total_packets();
            float rate      = csi_packet_rate();
            float amp       = csi_latest_amplitude();
            uint16_t sc     = csi_subcarrier_count();

            M5Cardputer.Display.printf("Pkts: %u total, %.1f Hz", total, rate);
            M5Cardputer.Display.setCursor(4, 100);
            M5Cardputer.Display.printf("SC: %u | Amp: %.1f", sc, amp);

            // Amplitude bar (right side, 0-128 range = 0-100%)
            int bar_w = (int)(amp * 200.0f / 128.0f);
            if (bar_w > 200) bar_w = 200;
            M5Cardputer.Display.fillRect(4, 116, bar_w, 6, TFT_CYAN);
            M5Cardputer.Display.fillRect(4 + bar_w, 116, 200 - bar_w, 6, TFT_DARKGREY);
        }

        // Connection status at bottom
        M5Cardputer.Display.setCursor(4, 128);
        if (WifiManager::isConnected()) {
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5Cardputer.Display.print("Connected");
        } else {
            M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
            M5Cardputer.Display.print("Disconnected");
        }
    }

    // Serial output every second
    static uint32_t last_serial = 0;
    if (now - last_serial >= 1000) {
        last_serial = now;
        if (g_csi_ok) {
            Serial.printf("[CSI] total=%u rate=%.1fHz amp=%.1f sc=%u\n",
                          csi_total_packets(),
                          csi_packet_rate(),
                          csi_latest_amplitude(),
                          csi_subcarrier_count());
        }
    }

    delay(10);
}
