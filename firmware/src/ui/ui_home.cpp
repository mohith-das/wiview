#include "ui_home.h"
#include "app/app_data.h"

namespace wiview {

void HomeScreen::enter() {
    m_next = ScreenId::HOME;
    M5Cardputer.Display.clear(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.println("wiview");

    // View label
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(160, 4);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("[1]");
}

void HomeScreen::update(const SensorData& d) {
    // Line 1: CSI health
    M5Cardputer.Display.fillRect(0, 28, 240, 10, TFT_BLACK);
    M5Cardputer.Display.setCursor(4, 28);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.printf("CSI %.1f Hz  sc:%u  p:%u", d.csi_rate_hz, d.subcarrier_count, d.total_packets);

    // Line 2: Presence
    M5Cardputer.Display.fillRect(0, 42, 240, 18, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    if (d.calibrated) {
        if (d.presence_detected) {
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5Cardputer.Display.setCursor(4, 42);
            M5Cardputer.Display.print("PRESENCE");
        } else {
            M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5Cardputer.Display.setCursor(4, 42);
            M5Cardputer.Display.print("CLEAR");
        }
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(140, 46);
        M5Cardputer.Display.printf("c:%.2f z:%.1f", d.presence_confidence, d.presence_z_score);
    } else {
        M5Cardputer.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 42);
        M5Cardputer.Display.print("CALIBRATING");
        M5Cardputer.Display.setTextSize(1);
        int pct = d.cal_samples * 100 / 600;
        if (pct > 100) pct = 100;
        M5Cardputer.Display.setCursor(140, 46);
        M5Cardputer.Display.printf("%d%%", pct);
    }

    // Line 3: Motion bar
    M5Cardputer.Display.fillRect(0, 64, 240, 10, TFT_BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setCursor(4, 64);
    M5Cardputer.Display.print("Motion");
    int bar_w = clamp_bar((int)(d.motion_level * 120.0f), 120);
    M5Cardputer.Display.fillRect(60, 64, bar_w, 7, TFT_GREEN);
    M5Cardputer.Display.fillRect(60 + bar_w, 64, 120 - bar_w, 7, TFT_DARKGREY);
    M5Cardputer.Display.setCursor(184, 64);
    M5Cardputer.Display.printf("%.2f", d.motion_level);

    // Line 4: IMU gate + breathing preview
    M5Cardputer.Display.fillRect(0, 78, 240, 10, TFT_BLACK);
    M5Cardputer.Display.setCursor(4, 78);
    if (d.device_still) {
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Cardputer.Display.print("STILL");
    } else {
        M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5Cardputer.Display.print("MOVING");
    }
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setCursor(70, 78);
    M5Cardputer.Display.printf("IMU a:%.2f g:%.1f", d.accel_magnitude, d.gyro_magnitude);

    // Breathing preview
    if (d.breathing_valid) {
        M5Cardputer.Display.setCursor(4, 90);
        M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5Cardputer.Display.printf("BPM: %.1f", d.breathing_bpm);
    }

    // Line 5: WiFi + streaming status
    M5Cardputer.Display.fillRect(0, 106, 240, 29, TFT_BLACK);
    M5Cardputer.Display.setCursor(4, 108);
    M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5Cardputer.Display.print("WiFi");

    M5Cardputer.Display.setCursor(40, 108);
    if (d.streaming) {
        // UDP is connectionless: this shows the destination + a live packet
        // count (climbing == we're actively sending), not a confirmed client.
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        IPAddress ip(d.stream_target_ip);
        M5Cardputer.Display.printf("STREAM>%s #%u", ip.toString().c_str(), d.stream_packets);
    } else {
        M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5Cardputer.Display.print("stream off (s)");
    }

    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.setCursor(4, 122);
    M5Cardputer.Display.print("1/2/3 views  r:cal  s:stream  h:host");
}

void HomeScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    for (char c : keys.word) {
        if (c == '2') { m_next = ScreenId::WATERFALL; return; }
        if (c == '3') { m_next = ScreenId::BREATHING; return; }
        if (c == 'h') { m_next = ScreenId::HOST_SETUP; return; }
    }
}

} // namespace wiview
