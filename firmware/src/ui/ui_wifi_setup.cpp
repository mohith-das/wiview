#include "ui_wifi_setup.h"
#include "app/app_data.h"

namespace wiview {

void WifiSetupScreen::enter() {
    m_next = ScreenId::HOME;
    m_ssid = "";
    m_password = "";
    m_enteringPassword = false;
    m_complete = false;
    m_dirty = true;
    renderPrompt();
}

void WifiSetupScreen::renderPrompt() {
    M5Cardputer.Display.clear(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.println("WiFi Setup");
    M5Cardputer.Display.setTextSize(1);

    if (!m_enteringPassword) {
        M5Cardputer.Display.setCursor(4, 36);
        M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5Cardputer.Display.print("SSID:");
        M5Cardputer.Display.setCursor(4, 52);
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Cardputer.Display.print(m_ssid);
    } else {
        M5Cardputer.Display.setCursor(4, 36);
        M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5Cardputer.Display.print("Password:");
        M5Cardputer.Display.setCursor(4, 52);
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        // Show * for each password character
        for (size_t i = 0; i < m_password.length(); i++) {
            M5Cardputer.Display.print("*");
        }
    }

    M5Cardputer.Display.setCursor(4, 84);
    M5Cardputer.Display.setTextColor(
        M5Cardputer.Keyboard.capslocked() ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.printf("CAPS: %s (fn toggles)",
        M5Cardputer.Keyboard.capslocked() ? "ON" : "off");

    M5Cardputer.Display.setCursor(4, 100);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("Enter=confirm  Del=backspace");
    M5Cardputer.Display.setCursor(4, 112);
    M5Cardputer.Display.print("Shift=caps  `=clear");
}

void WifiSetupScreen::update(const SensorData&) {
    // Screen is mostly static, redraws happen on key input
}

void WifiSetupScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    String& field = m_enteringPassword ? m_password : m_ssid;

    // Enter / Backspace / Space arrive as boolean flags on the Cardputer
    // keyboard, NOT as characters in keys.word.
    if (keys.enter) {
        if (!m_enteringPassword) {
            if (m_ssid.length() > 0) m_enteringPassword = true;  // SSID -> password
        } else if (m_password.length() > 0) {
            m_complete = true;  // password confirmed -> done
            return;
        }
        m_dirty = true;
    } else if (keys.del) {
        if (field.length() > 0) field.remove(field.length() - 1);
        m_dirty = true;
    } else if (keys.space) {
        field += ' ';
        m_dirty = true;
    } else if (keys.fn && keys.word.empty()) {
        // Standalone fn press toggles caps lock (no hardware caps key exists)
        M5Cardputer.Keyboard.setCapsLocked(!M5Cardputer.Keyboard.capslocked());
        m_dirty = true;
    } else {
        for (char c : keys.word) {
            if (c == '`') {
                // Backtick clears the current field
                field = "";
                m_dirty = true;
            } else if (c >= 32 && c <= 126) {
                field += c;
                m_dirty = true;
            }
        }
    }

    if (m_dirty) {
        m_dirty = false;
        renderPrompt();
    }
}

} // namespace wiview
