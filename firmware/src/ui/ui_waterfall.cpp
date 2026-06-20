#include "ui_waterfall.h"
#include "app/app_data.h"

namespace wiview {

void WaterfallScreen::enter() {
    m_next = ScreenId::WATERFALL;
    m_scroll_y = GRID_Y;
    M5Cardputer.Display.clear(TFT_BLACK);

    // Header
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 2);
    M5Cardputer.Display.println("Waterfall");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(140, 4);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("[2]");

    // Bottom hint
    M5Cardputer.Display.setCursor(4, 124);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("1:Home 2:Water 3:Breath");
}

void WaterfallScreen::update(const SensorData& d) {
    if (!d.amp_per_sc || d.num_sc == 0) return;

    // Determine columns: up to MAX_COLS, center
    int cols = d.num_sc > MAX_COLS ? MAX_COLS : (int)d.num_sc;
    int start_col = d.num_sc > (uint16_t)MAX_COLS ? ((int)d.num_sc - MAX_COLS) / 2 : 0;
    int x0 = (240 - cols * COL_W) / 2;

    // Scroll the waterfall up by 1 row
    m_scroll_y += 1;

    // Draw one row of subcarrier amplitudes as colored pixels
    for (int c = 0; c < cols; c++) {
        float amp = d.amp_per_sc[start_col + c];
        // Map amplitude to color: 0=dark, high=yellow/red
        int val = (int)(amp * 2.0f);  // amp ~0-128 → val 0-255
        if (val > 255) val = 255;
        uint16_t color = M5Cardputer.Display.color565(val, val/2, 0); // amber gradient

        M5Cardputer.Display.fillRect(x0 + c * COL_W, m_scroll_y, COL_W, 1, color);
    }

    // Reset scroll when reaching bottom
    if (m_scroll_y >= GRID_Y + GRID_H) {
        m_scroll_y = GRID_Y;
        M5Cardputer.Display.fillRect(0, GRID_Y, 240, GRID_H, TFT_BLACK);
    }

    // Packet rate at top of waterfall
    M5Cardputer.Display.fillRect(160, 4, 76, 12, TFT_BLACK);
    M5Cardputer.Display.setCursor(160, 4);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.printf("%.1f Hz", d.csi_rate_hz);
}

void WaterfallScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    for (char c : keys.word) {
        if (c == '1') { m_next = ScreenId::HOME; return; }
        if (c == '3') { m_next = ScreenId::BREATHING; return; }
    }
}

} // namespace wiview
