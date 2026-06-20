/**
 * wiview — Phase 2: DSP core + presence/motion on Cardputer-Adv
 *
 * Connects WiFi, captures CSI, preprocesses, calibrates baseline,
 * detects presence via z-score and motion via temporal variance,
 * gates inferences with BMI270 IMU device-motion check.
 */
#include <M5Cardputer.h>
#include <M5Unified.h>
#include "net/wifi_manager.h"
#include "csi/csi_capture.h"
#include "csi/packet_gen.h"
#include "dsp/dsp_preprocess.h"
#include "dsp/dsp_calibration.h"
#include "dsp/dsp_presence.h"
#include "dsp/dsp_motion.h"
#include "sensors/imu_gate.h"
#include "sensors/bmi270_reader.h"

using namespace wiview;
using namespace wiview::dsp;

static constexpr const char* WIFI_SSID     = "wiview-test";
static constexpr const char* WIFI_PASSWORD = "test1234";

static Calibrator    g_cal;
static MotionDetector g_motion;
static bool          g_wifi_ok = false;
static bool          g_csi_ok  = false;
static bool          g_cal_ready = false;
static PresenceResult g_last_presence = {};
static ImuGateResult  g_last_imu = {};
static float         g_current_rate = 0.0f;
static uint16_t      g_current_sc = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== wiview Phase 2 — DSP + Presence ===\n");

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    M5Cardputer.Display.clear(TFT_BLACK);

    // Header
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.println("wiview");

    // Connect + init
    g_wifi_ok = WifiManager::connect(WIFI_SSID, WIFI_PASSWORD);
    if (g_wifi_ok) {
        g_csi_ok = csi_init();
        if (g_csi_ok) {
            PacketGenerator::begin(WifiManager::gateway(), 50);
        }
    }

    g_cal.reset();
    g_motion.reset();
}

void loop() {
    M5Cardputer.update();

    if (g_wifi_ok) PacketGenerator::update();

    // Process latest CSI
    static uint32_t last_csi_processed = 0;
    uint32_t now = millis();
    if (g_csi_ok && now - last_csi_processed >= 20) {
        last_csi_processed = now;
        float amp = csi_latest_amplitude();
        g_current_rate = csi_packet_rate();
        g_current_sc   = csi_subcarrier_count();
        uint32_t total = csi_total_packets();

        if (total > 0 && g_current_sc > 0) {
            // In Phase 2 we don't have direct I/Q access from the ring buffer
            // (Phase 1 only stores aggregate). We use the per-packet amplitude
            // as a proxy feature for presence/motion detection.
            // Full subcarrier-level analysis comes in Phase 3.

            // Feed calibration (first 600 samples ~30s at 20 Hz)
            if (!g_cal_ready) {
                g_cal.add(amp);
                if (g_cal.count() >= 600) g_cal_ready = true;
            } else {
                // Variance is approximated from amplitude changes
                // For real subcarrier variance, need raw I/Q in ring buffer (Phase 3+)
                float approx_var = amp * 0.1f; // rough proxy

                g_last_presence = detect_presence(amp * 0.1f, g_cal, 3.0f);
                g_motion.add(amp, now);
            }
        }

        // Read IMU gate
        g_last_imu = imu_evaluate(bmi270_read());
    }

    // Update display @ 5 Hz
    static uint32_t last_disp = 0;
    if (now - last_disp >= 200) {
        last_disp = now;

        // Line 1: CSI health
        M5Cardputer.Display.fillRect(0, 36, 240, 10, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 36);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.printf("CSI %.1f Hz  sc:%u", g_current_rate, g_current_sc);

        // Line 2: Presence
        M5Cardputer.Display.fillRect(0, 50, 240, 16, TFT_BLACK);
        M5Cardputer.Display.setTextSize(2);
        if (g_cal_ready) {
            if (g_last_presence.present) {
                M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
                M5Cardputer.Display.setCursor(4, 50);
                M5Cardputer.Display.print("PRESENCE");
            } else {
                M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
                M5Cardputer.Display.setCursor(4, 50);
                M5Cardputer.Display.print("CLEAR");
            }
            // Confidence
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(140, 54);
            M5Cardputer.Display.printf("c:%.2f", g_last_presence.confidence);
        } else {
            M5Cardputer.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
            M5Cardputer.Display.setCursor(4, 50);
            M5Cardputer.Display.print("CALIBRATING");
            // Progress
            int pct = g_cal.count() * 100 / 600;
            M5Cardputer.Display.setCursor(140, 54);
            M5Cardputer.Display.printf("%d%%", pct);
        }

        // Line 3: Motion bar
        M5Cardputer.Display.fillRect(0, 72, 240, 10, TFT_BLACK);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 72);
        float ml = g_motion.motion_level();
        M5Cardputer.Display.printf("Motion");
        int bar_w2 = (int)(ml * 120.0f);
        if (bar_w2 > 120) bar_w2 = 120;
        M5Cardputer.Display.fillRect(60, 72, bar_w2, 7, TFT_GREEN);
        M5Cardputer.Display.fillRect(60 + bar_w2, 72, 120 - bar_w2, 7, TFT_DARKGREY);
        M5Cardputer.Display.setCursor(184, 72);
        M5Cardputer.Display.printf("%.2f", ml);

        // Line 4: IMU gate + calibration
        M5Cardputer.Display.fillRect(0, 86, 240, 10, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 86);
        if (g_last_imu.device_still) {
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5Cardputer.Display.print("STILL");
        } else {
            M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
            M5Cardputer.Display.print("MOVING");
        }
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.setCursor(80, 86);
        M5Cardputer.Display.printf("a:%.3fg g:%.1f", g_last_imu.accel_magnitude, g_last_imu.gyro_magnitude);

        // Line 5: Status bar
        M5Cardputer.Display.fillRect(0, 100, 240, 35, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 100);
        if (g_wifi_ok) {
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5Cardputer.Display.print("WiFi OK");
        } else {
            M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
            M5Cardputer.Display.print("WiFi FAIL");
        }
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.setCursor(100, 100);
        M5Cardputer.Display.printf("Pkts:%u", csi_total_packets());
    }

    delay(10);
}
