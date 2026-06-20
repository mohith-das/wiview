#include "app_controller.h"
#include "ui/ui_home.h"
#include "ui/ui_waterfall.h"
#include "ui/ui_breathing.h"
#include "ui/ui_wifi_setup.h"
#include "ui/ui_host_setup.h"
#include "net/wifi_manager.h"
#include "net/wifi_provision.h"
#include "net/udp_streamer.h"
#include "net/host_discovery.h"
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

static Calibrator        g_cal;
static MotionDetector    g_motion;
static BreathingDetector g_breath;
static UdpStreamer       g_streamer;
static WifiSetupScreen*  g_wifiSetup = nullptr;
static bool              g_wifi_ok = false;
static bool              g_csi_ok  = false;
static bool              g_cal_ready = false;
static bool              g_streaming = false;
static float             g_amp_sc_buffer[MAX_SUBCARRIERS] = {};
static IPAddress         g_host_ip;          // 0.0.0.0 = none (discovered / NVS / manual)
static uint16_t          g_host_port = 5005;

// Resolve the UDP streaming target, in priority order:
//   1. a host learned via discovery / NVS / manual entry (g_host_ip)
//   2. the WIVIEW_STREAM_HOST build flag, if set and parseable
//   3. the gateway (last-resort default)
static IPAddress streamTarget() {
    if ((uint32_t)g_host_ip != 0) return g_host_ip;
#ifdef WIVIEW_STREAM_HOST
    IPAddress ip;
    if (ip.fromString(WIVIEW_STREAM_HOST)) return ip;
#endif
    return WifiManager::gateway();
}

static uint16_t streamPort() { return g_host_port; }

AppController::AppController() : m_current(ScreenId::HOME) {
    for (int i = 0; i < (int)ScreenId::COUNT; i++) m_screens[i] = nullptr;
    m_screensWifi = nullptr;
}

void AppController::begin() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== wiview Phase 4 — Config + Streaming ===\n");

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    initScreens();

    // Attempt WiFi with saved credentials
    String ssid, pass;
    bool hasCreds = WifiProvision::loadFromNVS(ssid, pass);
    if (hasCreds) {
        g_wifi_ok = WifiManager::connect(ssid.c_str(), pass.c_str());
    }

    if (!g_wifi_ok) {
        // Show WiFi setup screen
        m_screensWifi = new WifiSetupScreen();
        m_current = ScreenId::HOME; // will be overridden below
        showWifiSetup();
        return;
    }

    finishInit();
}

void AppController::initScreens() {
    m_screens[(int)ScreenId::HOME]       = new HomeScreen();
    m_screens[(int)ScreenId::WATERFALL]  = new WaterfallScreen();
    m_screens[(int)ScreenId::BREATHING]  = new BreathingScreen();
    m_screens[(int)ScreenId::HOST_SETUP] = new HostSetupScreen();
}

void AppController::finishInit() {
    g_csi_ok = csi_init();
    if (g_csi_ok) {
        PacketGenerator::begin(WifiManager::gateway(), 50);
    }
    g_cal.reset();
    g_motion.reset();
    g_breath.reset();
    m_data.amp_per_sc = g_amp_sc_buffer;

    // Stream-host resolution: a previously saved host (discovered or manually
    // entered) wins; otherwise discovery beacons / the build flag / gateway.
    String savedHost = WifiProvision::loadHostFromNVS();
    if (savedHost.length() > 0) {
        IPAddress ip;
        if (ip.fromString(savedHost)) {
            g_host_ip = ip;
            Serial.printf("[Host] saved stream host = %s\n", savedHost.c_str());
        }
    }
    HostDiscovery::begin();

    // Output format: wiview native (for the host companion / bridge) or RuView
    // ADR-018 direct. Persisted across reboots.
    bool rv = WifiProvision::loadRuViewModeFromNVS();
    g_streamer.setRuViewMode(rv);
    m_data.ruview_mode = rv;
    Serial.printf("[Fmt] output = %s\n", rv ? "RuView ADR-018" : "wiview native");

    m_screens[(int)m_current]->enter();
    // Streaming is OFF by default; user toggles with 's' key.
}

// Apply a newly learned stream host (from discovery or manual entry): persist it
// and, if a stream is already running, re-point it at the new target.
void AppController::setStreamHost(const IPAddress& host, uint16_t port) {
    bool changed = ((uint32_t)host != (uint32_t)g_host_ip) || (port != g_host_port);
    g_host_ip   = host;
    g_host_port = port;
    if (!changed) return;

    WifiProvision::saveHostToNVS(host.toString());
    Serial.printf("[Host] stream host -> %s:%u\n", host.toString().c_str(), port);
    if (g_streaming) {
        g_streamer.stop();
        g_streaming = g_streamer.begin(streamTarget(), streamPort());
        m_data.streaming = g_streaming;
        m_data.stream_target_ip = g_streaming ? (uint32_t)g_streamer.target() : 0;
    }
}

void AppController::pollDiscovery() {
    IPAddress host;
    uint16_t  port = 0;
    if (HostDiscovery::poll(host, port)) {
        setStreamHost(host, port);
    }
}

void AppController::showWifiSetup() {
    if (!m_screensWifi) return;
    m_screensWifi->enter();
    // Run WiFi setup loop until complete
    while (!m_screensWifi->isComplete()) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto& keys = M5Cardputer.Keyboard.keysState();
            m_screensWifi->handleKey(keys);
        }
        delay(20);
    }
    String ssid = m_screensWifi->ssid();
    String pass = m_screensWifi->password();
    delete m_screensWifi;
    m_screensWifi = nullptr;

    WifiProvision::saveToNVS(ssid, pass);
    g_wifi_ok = WifiManager::connect(ssid.c_str(), pass.c_str());
    if (!g_wifi_ok) {
        // Retry on failure
        Serial.println("[WiFi] Connection failed. Restarting in 5s...");
        delay(5000);
        ESP.restart();
    }
    finishInit();
}

void AppController::collectData() {
    static uint32_t last_csi = 0;
    uint32_t now = millis();
    if (!g_csi_ok || now - last_csi < 20) return;
    last_csi = now;

    m_data.streaming      = g_streaming;
    m_data.stream_packets = g_streaming ? g_streamer.packetsSent() : 0;

    float amp = csi_latest_amplitude();
    m_data.csi_rate_hz     = csi_packet_rate();
    m_data.subcarrier_count = csi_subcarrier_count();
    m_data.total_packets   = csi_total_packets();
    m_data.latest_amplitude = amp;

    // Streaming: send each freshly-captured raw CSI I/Q frame once.
    if (g_streaming && g_streamer.isActive()) {
        static int8_t  iq[512];
        static uint32_t last_iq_seq = 0;
        uint16_t iq_len = 0;
        uint32_t iq_seq = 0;
        if (csi_latest_iq(iq, sizeof(iq), &iq_len, &iq_seq) &&
            iq_len > 0 && iq_seq != last_iq_seq) {
            last_iq_seq = iq_seq;
            g_streamer.sendCsiPacket(iq, iq_len, now);
        }
    }

    if (m_data.total_packets > 0 && m_data.subcarrier_count > 0) {
        if (!g_cal_ready) {
            g_cal.add(amp);
            m_data.cal_samples = g_cal.count();
            if (g_cal.count() >= 600) g_cal_ready = true;
        } else {
            m_data.calibrated = true;
            float approx_var = amp * 0.1f;
            auto pres = detect_presence(approx_var, g_cal, 3.0f);
            m_data.presence_detected  = pres.present;
            m_data.presence_confidence = pres.confidence;
            m_data.presence_z_score   = pres.z_score;

            g_motion.add(amp, now);
            m_data.motion_level = g_motion.motion_level();

            // Feed the breathing detector at its expected ~20 Hz rate (the
            // collect loop runs faster); its BPM math assumes 20 Hz samples.
            static uint32_t last_breath_ms = 0;
            if (now - last_breath_ms >= 50) {
                last_breath_ms = now;
                g_breath.add(amp);
            }
            m_data.breathing_bpm   = g_breath.bpm();
            m_data.breathing_waveform = g_breath.waveform();
            m_data.breathing_valid = g_breath.valid();

            // Send inferences at ~2 Hz — RuView vitals (ADR-018) in RuView mode,
            // wiview JSON otherwise.
            static uint32_t last_inf = 0;
            if (g_streaming && now - last_inf >= 500) {
                last_inf = now;
                if (g_streamer.ruViewMode()) {
                    g_streamer.sendVitals(m_data.presence_detected, m_data.motion_level,
                                          m_data.breathing_bpm, now);
                } else {
                    char json[256];
                    snprintf(json, sizeof(json),
                        "{\"t\":%u,\"presence\":%s,\"conf\":%.2f,\"motion\":%.2f,"
                        "\"bpm\":%.1f,\"still\":%s,\"csi_hz\":%.1f}",
                        now,
                        m_data.presence_detected ? "true" : "false",
                        m_data.presence_confidence,
                        m_data.motion_level,
                        m_data.breathing_bpm,
                        m_data.device_still ? "true" : "false",
                        m_data.csi_rate_hz);
                    g_streamer.sendInference(json);
                }
            }

            uint16_t ns = m_data.subcarrier_count;
            if (ns > MAX_SUBCARRIERS) ns = MAX_SUBCARRIERS;
            m_data.num_sc = ns;
            for (uint16_t i = 0; i < ns; i++) {
                m_data.amp_per_sc[i] = amp;
            }
        }

        auto imu_sample = bmi270_read();
        auto imu_gated  = imu_evaluate(imu_sample);
        m_data.device_still     = imu_gated.device_still;
        m_data.accel_magnitude  = imu_gated.accel_magnitude;
        m_data.gyro_magnitude   = imu_gated.gyro_magnitude;
    }
}

void AppController::dispatchInput() {
    M5Cardputer.update();

    // Expire the "forget WiFi" confirmation prompt after 5 s.
    if (m_wifiForgetArmedMs != 0 && millis() - m_wifiForgetArmedMs >= 5000) {
        m_wifiForgetArmedMs = 0;
        m_data.wifi_forget_armed = false;
    }

    if (M5.BtnA.wasPressed()) {
        // G0 cycles only the main views (Home/Waterfall/Breathing), not config
        // screens like Host setup.
        int base = (int)m_current;
        int next = (base >= NUM_MAIN_SCREENS) ? 0 : (base + 1) % NUM_MAIN_SCREENS;
        m_current = (ScreenId)next;
        m_screens[(int)m_current]->enter();
        return;
    }

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto& keys = M5Cardputer.Keyboard.keysState();

        // Global keys
        for (char c : keys.word) {
            if (c == 's') {
                // Toggle streaming
                if (!g_streaming) {
                    g_streaming = g_streamer.begin(streamTarget(), streamPort());
                } else {
                    g_streamer.stop();
                    g_streaming = false;
                }
                m_data.streaming = g_streaming;
                m_data.stream_target_ip = g_streaming ? (uint32_t)g_streamer.target() : 0;
                return;
            }
            if (c == 'r') {
                // Recalibrate
                g_cal.reset();
                g_cal_ready = false;
                return;
            }
            if (c == 'u') {
                // Toggle output format: wiview native <-> RuView ADR-018 direct
                bool rv = !g_streamer.ruViewMode();
                g_streamer.setRuViewMode(rv);
                m_data.ruview_mode = rv;
                WifiProvision::saveRuViewModeToNVS(rv);
                Serial.printf("[Fmt] output = %s\n", rv ? "RuView ADR-018" : "wiview native");
                return;
            }
            if (c == 'w') {
                // Forget WiFi — two-press confirm to avoid an accidental wipe.
                uint32_t now = millis();
                if (m_wifiForgetArmedMs != 0 && now - m_wifiForgetArmedMs < 5000) {
                    Serial.println("[WiFi] Forgetting credentials. Rebooting to setup...");
                    WifiProvision::clearWifiCreds();
                    delay(300);
                    ESP.restart();
                } else {
                    m_wifiForgetArmedMs = now;
                    m_data.wifi_forget_armed = true;
                }
                return;
            }
        }

        m_screens[(int)m_current]->handleKey(keys);
        ScreenId next = m_screens[(int)m_current]->nextScreen();
        if (next != m_current) {
            // Apply a manually-entered host IP when leaving the Host setup screen.
            if (m_current == ScreenId::HOST_SETUP) {
                IPAddress entered;
                auto* hs = static_cast<HostSetupScreen*>(m_screens[(int)ScreenId::HOST_SETUP]);
                if (hs->takeCommitted(entered)) {
                    setStreamHost(entered, streamPort());
                }
            }
            m_current = next;
            m_screens[(int)m_current]->enter();
        }
    }
}

void AppController::update() {
    static uint32_t last_disp = 0;
    uint32_t now = millis();

    PacketGenerator::update();
    pollDiscovery();
    collectData();
    dispatchInput();

    if (now - last_disp >= 200) {
        last_disp = now;
        m_screens[(int)m_current]->update(m_data);
    }

    delay(10);
}

} // namespace wiview
