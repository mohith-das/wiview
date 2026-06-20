#pragma once
#include "ui_screen.h"
#include <Arduino.h>

namespace wiview {

/// WiFi credential entry screen using the keyboard.
class WifiSetupScreen : public Screen {
public:
    void enter() override;
    void update(const struct SensorData& data) override;
    void handleKey(const Keyboard_Class::KeysState& keys) override;
    ScreenId nextScreen() override { return m_next; }

    /// Check if provisioning is complete (both SSID and password entered)
    bool isComplete() const { return m_complete; }

    /// Get entered credentials
    String ssid() const { return m_ssid; }
    String password() const { return m_password; }

private:
    ScreenId m_next = ScreenId::HOME;
    String m_ssid, m_password;
    bool m_enteringPassword = false;
    bool m_complete = false;
    bool m_dirty = true;

    void renderPrompt();
};

} // namespace wiview
