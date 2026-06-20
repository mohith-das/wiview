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

    M5Cardputer.Display.setCursor(4, 100);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.print("Enter=confirm  Esc=clear");
}

void WifiSetupScreen::update(const SensorData&) {
    // Screen is mostly static, redraws happen on key input
}

void WifiSetupScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    for (char c : keys.word) {
        if (c == KEY_ENTER || c == '\n' || c == '\r') {
            if (!m_enteringPassword) {
                // Confirm SSID
                if (m_ssid.length() > 0) {
                    m_enteringPassword = true;
                }
            } else {
                // Confirm password -> provisioning complete
                if (m_password.length() > 0) {
                    m_complete = true;
                    return;
                }
            }
            m_dirty = true;
        } else if (c == KEY_BACKSPACE || c == 8 || c == 127) {
            if (!m_enteringPassword) {
                if (m_ssid.length() > 0) m_ssid.remove(m_ssid.length() - 1);
            } else {
                if (m_password.length() > 0) m_password.remove(m_password.length() - 1);
            }
            m_dirty = true;
        } else if (c == c == 27) {
            if (m_enteringPassword) {
                m_password = "";
            }
            m_ssid = "";
            m_enteringPassword = false;
            m_dirty = true;
        } else if (c >= 32 && c <= 126) {
            // Printable character
            if (!m_enteringPassword) {
                m_ssid += c;
            } else {
                m_password += c;
            }
            m_dirty = true;
        }
    }

    if (m_dirty) {
        m_dirty = false;
        renderPrompt();
    }
}

} // namespace wiview
