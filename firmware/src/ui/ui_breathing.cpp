#include "ui_breathing.h"
#include "app/app_data.h"

namespace wiview {

void BreathingScreen::enter() {
    m_next = ScreenId::BREATHING;
    m_wave_idx = 0;
    for (int i = 0; i < WAVEFORM_W; i++) m_wave_history[i] = 0.0f;

    M5Cardputer.Display.clear(TFT_BLACK);

    // Header
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 2);
    M5Cardputer.Display.println("Breathing");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(140, 4);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("[3]");

    // Waveform bounding box
    M5Cardputer.Display.drawRect(0, WAVEFORM_Y - 1, WAVEFORM_W + 2, WAVEFORM_H + 8, TFT_DARKGREY);

    // Bottom hint
    M5Cardputer.Display.setCursor(4, 124);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("1:Home 2:Water 3:Breath");
}

void BreathingScreen::update(const SensorData& d) {
    // Store waveform point
    m_wave_history[m_wave_idx] = d.breathing_waveform;
    m_wave_idx = (m_wave_idx + 1) % WAVEFORM_W;

    // Draw BPM on the right
    M5Cardputer.Display.fillRect(185, WAVEFORM_Y, 55, WAVEFORM_H, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    if (d.breathing_valid) {
        M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5Cardputer.Display.setCursor(186, WAVEFORM_Y + 10);
        M5Cardputer.Display.printf("%.0f", d.breathing_bpm);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(186, WAVEFORM_Y + 36);
        M5Cardputer.Display.print("BPM");
    } else {
        M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5Cardputer.Display.setCursor(186, WAVEFORM_Y + 20);
        M5Cardputer.Display.print("--");
        M5Cardputer.Display.setCursor(186, WAVEFORM_Y + 36);
        M5Cardputer.Display.print("BPM");
    }

    // Draw waveform
    int mid_y = WAVEFORM_Y + WAVEFORM_H / 2;
    int amp_scale = 30;  // waveform value range

    // Clear waveform area (left side)
    M5Cardputer.Display.fillRect(1, WAVEFORM_Y, WAVEFORM_W, WAVEFORM_H, TFT_BLACK);

    // Draw line segments
    for (int i = 0; i < WAVEFORM_W - 1; i++) {
        int idx1 = (m_wave_idx - WAVEFORM_W + i + WAVEFORM_W * 2) % WAVEFORM_W;
        int idx2 = (idx1 + 1) % WAVEFORM_W;
        int y1 = mid_y - (int)(m_wave_history[idx1] * amp_scale);
        int y2 = mid_y - (int)(m_wave_history[idx2] * amp_scale);
        if (y1 < WAVEFORM_Y) y1 = WAVEFORM_Y;
        if (y1 > WAVEFORM_Y + WAVEFORM_H) y1 = WAVEFORM_Y + WAVEFORM_H;
        if (y2 < WAVEFORM_Y) y2 = WAVEFORM_Y;
        if (y2 > WAVEFORM_Y + WAVEFORM_H) y2 = WAVEFORM_Y + WAVEFORM_H;
        M5Cardputer.Display.drawLine(i, y1, i + 1, y2, TFT_GREEN);
    }

    // CSI rate
    M5Cardputer.Display.fillRect(160, 4, 76, 12, TFT_BLACK);
    M5Cardputer.Display.setCursor(160, 4);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.printf("%.1f Hz", d.csi_rate_hz);
}

void BreathingScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    for (char c : keys.word) {
        if (c == '1') { m_next = ScreenId::HOME; return; }
        if (c == '2') { m_next = ScreenId::WATERFALL; return; }
    }
}

} // namespace wiview
