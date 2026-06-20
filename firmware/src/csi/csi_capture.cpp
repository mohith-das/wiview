/**
 * wiview — CSI capture for ESP32-S3
 *
 * Registers an esp_wifi_set_csi_rx_cb() callback that copies CSI I/Q data
 * into a volatile ring buffer. Exposes packet count, smoothed rate, and
 * latest amplitude for the UI.
 *
 * Data layout per ESP32-S3 LLTF (802.11n mixed mode, non-HT duplicate):
 *   Each subcarrier: 1 byte I (signed), 1 byte Q (signed)
 *   Total subcarriers: len bytes / 2
 *   Typical: 52 subcarriers (104 bytes) for 20 MHz HT, 128 for 40 MHz
 */
#include "csi_capture.h"
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace wiview {

// ── Ring buffer ──────────────────────────────────────────────────────────
static constexpr size_t kTimestampWindow = 64;
// Max raw I/Q bytes per CSI record. LLTF @ HT20 is ~128 bytes (64 subcarriers);
// 512 covers HT40 / merged-LTF with margin.
static constexpr size_t kMaxIqBytes = 512;

static volatile uint32_t g_packet_count   = 0;
static volatile float    g_latest_amp     = 0.0f;
static volatile uint16_t g_subcarrier_cnt = 0;
static volatile uint32_t g_timestamps[kTimestampWindow] = {};
static volatile uint8_t  g_ts_idx         = 0;

// Latest raw I/Q snapshot (written by the RX callback, read by the app task).
static int8_t            g_iq_buf[kMaxIqBytes];
static volatile uint16_t g_iq_len         = 0;
static volatile uint32_t g_iq_seq         = 0;

// ── CSI RX callback (runs in WiFi task — keep FAST) ──────────────────────
static void IRAM_ATTR csi_rx_cb(void* ctx, wifi_csi_info_t* info) {
    // info->buf points to raw I/Q byte pairs (int8_t I, int8_t Q, ...)
    int num_pairs = info->len / 2;
    if (num_pairs == 0) return;

    // Compute mean amplitude: 1/N * sum(sqrt(I^2 + Q^2))
    int64_t sum_sq = 0;
    const int8_t* p = info->buf;
    for (int i = 0; i < num_pairs; i++) {
        int16_t i_val = p[0];
        int16_t q_val = p[1];
        sum_sq += i_val * i_val + q_val * q_val;
        p += 2;
    }
    float mean_amp = sqrtf((float)sum_sq / num_pairs);

    g_latest_amp     = mean_amp;
    g_subcarrier_cnt = num_pairs;

    // Snapshot the raw I/Q bytes so the app task can stream them downstream
    // (e.g. to the host / RuView). info->buf is valid only during this call.
    uint16_t n = (uint16_t)info->len;
    if (n > kMaxIqBytes) n = kMaxIqBytes;
    memcpy(g_iq_buf, info->buf, n);
    g_iq_len = n;
    g_iq_seq++;

    // Rolling timestamp for packet rate
    g_timestamps[g_ts_idx % kTimestampWindow] = (uint32_t)millis();
    g_ts_idx++;

    g_packet_count++;
}

// ── Public API ───────────────────────────────────────────────────────────

bool csi_init() {
    // Configure CSI: LLTF only (no HT-LTF to keep it simple for Phase 1)
    wifi_csi_config_t cfg = {};
    cfg.lltf_en           = 1;   // enable Legacy Long Training Field
    cfg.htltf_en          = 0;   // disable HT-LTF for now
    cfg.stbc_htltf2_en    = 0;
    cfg.ltf_merge_en      = 1;   // merge LTF to get cleaner data
    cfg.channel_filter_en = 0;
    cfg.manu_scale        = 0;
    cfg.shift             = 0;

    esp_err_t ret = esp_wifi_set_csi_config(&cfg);
    if (ret != ESP_OK) {
        Serial.printf("[CSI] esp_wifi_set_csi_config failed: 0x%x\n", ret);
        return false;
    }

    ret = esp_wifi_set_csi_rx_cb(csi_rx_cb, nullptr);
    if (ret != ESP_OK) {
        Serial.printf("[CSI] esp_wifi_set_csi_rx_cb failed: 0x%x\n", ret);
        return false;
    }

    ret = esp_wifi_set_csi(true);
    if (ret != ESP_OK) {
        Serial.printf("[CSI] esp_wifi_set_csi failed: 0x%x\n", ret);
        return false;
    }

    Serial.println("[CSI] Initialized — waiting for packets...");
    return true;
}

uint32_t csi_total_packets() {
    return g_packet_count;
}

float csi_packet_rate() {
    // Count timestamps within the last 2000 ms
    uint32_t now = millis();
    uint32_t count = 0;
    uint32_t oldest = UINT32_MAX;
    for (size_t i = 0; i < kTimestampWindow; i++) {
        uint32_t t = g_timestamps[i];
        if (t == 0) continue;          // slot never filled
        uint32_t age = now - t;
        if (age < 2000) {
            count++;
            if (t < oldest) oldest = t;
        }
    }
    if (count < 2 || oldest >= now) return 0.0f;

    float window_sec = (now - oldest) / 1000.0f;
    if (window_sec < 0.1f) return 0.0f;
    return (float)(count - 1) / window_sec;  // -1 to exclude edge
}

float csi_latest_amplitude() {
    return g_latest_amp;
}

uint16_t csi_subcarrier_count() {
    return g_subcarrier_cnt;
}

bool csi_latest_iq(int8_t* out, uint16_t cap, uint16_t* out_len, uint32_t* out_seq) {
    if (!out || cap == 0) return false;
    // Seqlock-style retry to minimise tearing against the RX callback.
    for (int attempt = 0; attempt < 3; attempt++) {
        uint32_t s1 = g_iq_seq;
        uint16_t n  = g_iq_len;
        if (n == 0) return false;
        if (n > cap) n = cap;
        memcpy(out, g_iq_buf, n);
        if (g_iq_seq == s1) {           // no write happened mid-copy
            *out_len = n;
            *out_seq = s1;
            return true;
        }
    }
    return false;
}

} // namespace wiview
