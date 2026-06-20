#include "ui_host_setup.h"
#include "app/app_data.h"
#include "net/wifi_provision.h"

namespace wiview {

void HostSetupScreen::enter() {
    m_next      = ScreenId::HOST_SETUP;
    m_ip        = WifiProvision::loadHostFromNVS();  // prefill with current host
    m_committed = false;
    m_invalid   = false;
    m_dirty     = true;
    render();
}

void HostSetupScreen::render() {
    auto& d = M5Cardputer.Display;
    d.clear(TFT_BLACK);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.setTextSize(2);
    d.setCursor(4, 4);
    d.println("Host IP");

    d.setTextSize(1);
    d.setCursor(4, 36);
    d.setTextColor(TFT_CYAN, TFT_BLACK);
    d.print("Stream CSI to:");
    d.setCursor(4, 52);
    d.setTextColor(m_invalid ? TFT_RED : TFT_GREEN, TFT_BLACK);
    d.print(m_ip.length() ? m_ip.c_str() : "(empty)");

    if (m_invalid) {
        d.setCursor(4, 68);
        d.setTextColor(TFT_RED, TFT_BLACK);
        d.print("invalid IPv4");
    }

    d.setCursor(4, 84);
    d.setTextColor(TFT_DARKGREY, TFT_BLACK);
    d.print("Auto-discovery also runs in");
    d.setCursor(4, 94);
    d.print("the background when available.");

    d.setCursor(4, 110);
    d.print("0-9 . type  Enter save  `clear");
    d.setCursor(4, 122);
    d.print("G0 = back");
}

void HostSetupScreen::update(const SensorData&) {
    // Static screen; redraws happen on key input.
}

void HostSetupScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    if (keys.enter) {
        IPAddress ip;
        if (m_ip.length() > 0 && ip.fromString(m_ip)) {
            m_committedIp = ip;
            m_committed   = true;
            m_next        = ScreenId::HOME;
        } else {
            m_invalid = true;
            m_dirty   = true;
        }
    } else if (keys.del) {
        if (m_ip.length() > 0) m_ip.remove(m_ip.length() - 1);
        m_invalid = false;
        m_dirty   = true;
    } else {
        for (char c : keys.word) {
            if (c == '`') {
                m_ip = "";
                m_invalid = false;
                m_dirty   = true;
            } else if ((c >= '0' && c <= '9') || c == '.') {
                m_ip += c;
                m_invalid = false;
                m_dirty   = true;
            }
        }
    }

    if (m_dirty) {
        m_dirty = false;
        render();
    }
}

bool HostSetupScreen::takeCommitted(IPAddress& out) {
    if (!m_committed) return false;
    out         = m_committedIp;
    m_committed = false;
    return true;
}

} // namespace wiview
