#include "app_controller.h"
#include "ui/ui_home.h"
#include "ui/ui_waterfall.h"
#include "ui/ui_breathing.h"
#include "net/wifi_manager.h"
#include "csi/csi_capture.h"
#include "csi/packet_gen.h"
#include "dsp/dsp_preprocess.h"
#include "dsp/dsp_calibration.h"
#include "dsp/dsp_presence.h"
#include "dsp/dsp_motion.h"
#include "dsp/dsp_breathing.h"
#include "sensors/imu_gate.h"
#include "sensors/bmi270_reader.h"

using namespace wiview::dsp;

namespace wiview {

static constexpr const char* WIFI_SSID     = "wiview-test";
static constexpr const char* WIFI_PASSWORD = "test1234";

// Module state
static Calibrator        g_cal;
static MotionDetector    g_motion;
static BreathingDetector g_breath;
static bool              g_wifi_ok = false;
static bool              g_csi_ok  = false;
static bool              g_cal_ready = false;
static float             g_amp_sc_buffer[MAX_SUBCARRIERS] = {};

AppController::AppController() : m_current(ScreenId::HOME) {
    m_screens[0] = nullptr;
    m_screens[1] = nullptr;
    m_screens[2] = nullptr;
}

void AppController::begin() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== wiview Phase 3 — Breathing + UI ===\n");

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    initScreens();

    // Connect WiFi + init CSI
    g_wifi_ok = WifiManager::connect(WIFI_SSID, WIFI_PASSWORD);
    if (g_wifi_ok) {
        g_csi_ok = csi_init();
        if (g_csi_ok) {
            PacketGenerator::begin(WifiManager::gateway(), 50);
        }
    }

    g_cal.reset();
    g_motion.reset();
    g_breath.reset();
    m_data.amp_per_sc = g_amp_sc_buffer;

    m_screens[(int)m_current]->enter();
}

void AppController::initScreens() {
    m_screens[0] = new HomeScreen();
    m_screens[1] = new WaterfallScreen();
    m_screens[2] = new BreathingScreen();
}

void AppController::collectData() {
    static uint32_t last_csi = 0;
    uint32_t now = millis();
    if (!g_csi_ok || now - last_csi < 20) return;
    last_csi = now;

    float amp = csi_latest_amplitude();
    m_data.csi_rate_hz     = csi_packet_rate();
    m_data.subcarrier_count = csi_subcarrier_count();
    m_data.total_packets   = csi_total_packets();
    m_data.latest_amplitude = amp;

    if (m_data.total_packets > 0 && m_data.subcarrier_count > 0) {
        // Calibration
        if (!g_cal_ready) {
            g_cal.add(amp);
            m_data.cal_samples = g_cal.count();
            if (g_cal.count() >= 600) g_cal_ready = true;
        } else {
            m_data.calibrated = true;
            float approx_var = amp * 0.1f;

            // Presence
            auto pres = detect_presence(approx_var, g_cal, 3.0f);
            m_data.presence_detected  = pres.present;
            m_data.presence_confidence = pres.confidence;
            m_data.presence_z_score   = pres.z_score;

            // Motion
            g_motion.add(amp, now);
            m_data.motion_level = g_motion.motion_level();

            // Breathing
            g_breath.add(amp);
            m_data.breathing_bpm   = g_breath.bpm();
            m_data.breathing_waveform = g_breath.waveform();
            m_data.breathing_valid = g_breath.valid();

            // Waterfall: read raw I/Q from latest CSI sample
            uint16_t ns = m_data.subcarrier_count;
            if (ns > MAX_SUBCARRIERS) ns = MAX_SUBCARRIERS;
            m_data.num_sc = ns;
            for (uint16_t i = 0; i < ns; i++) {
                m_data.amp_per_sc[i] = amp;  // placeholder — replace with real per-SC data
            }
        }

        // IMU
        auto imu_sample = bmi270_read();
        auto imu_gated  = imu_evaluate(imu_sample);
        m_data.device_still     = imu_gated.device_still;
        m_data.accel_magnitude  = imu_gated.accel_magnitude;
        m_data.gyro_magnitude   = imu_gated.gyro_magnitude;
    }
}

void AppController::dispatchInput() {
    M5Cardputer.update();

    // G0 button cycles screens
    if (M5.BtnA.wasPressed()) {
        int next = ((int)m_current + 1) % (int)ScreenId::COUNT;
        m_current = (ScreenId)next;
        m_screens[(int)m_current]->enter();
        return;
    }

    // Keyboard
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto& keys = M5Cardputer.Keyboard.keysState();
        m_screens[(int)m_current]->handleKey(keys);

        ScreenId next = m_screens[(int)m_current]->nextScreen();
        if (next != m_current) {
            m_current = next;
            m_screens[(int)m_current]->enter();
        }
    }
}

void AppController::update() {
    static uint32_t last_disp = 0;
    uint32_t now = millis();

    PacketGenerator::update();
    collectData();
    dispatchInput();

    // Redraw at ~5 Hz
    if (now - last_disp >= 200) {
        last_disp = now;
        m_screens[(int)m_current]->update(m_data);
    }

    delay(10);
}

} // namespace wiview
