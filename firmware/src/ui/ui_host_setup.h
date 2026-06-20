#pragma once
#include "ui_screen.h"
#include <WiFi.h>

namespace wiview {

/// Manual stream-host entry (fallback for networks that block discovery
/// broadcasts). Type an IPv4 address; on Enter it's validated and committed.
class HostSetupScreen : public Screen {
public:
    void enter() override;
    void update(const SensorData& data) override;
    void handleKey(const Keyboard_Class::KeysState& keys) override;
    ScreenId nextScreen() override { return m_next; }

    /// If a valid IP was committed since the last call, copy it out, clear the
    /// flag, and return true.
    bool takeCommitted(IPAddress& out);

private:
    void render();
    String   m_ip;
    ScreenId m_next = ScreenId::HOST_SETUP;
    IPAddress m_committedIp;
    bool m_committed = false;
    bool m_invalid   = false;
    bool m_dirty     = true;
};

} // namespace wiview
