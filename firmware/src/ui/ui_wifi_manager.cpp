#include "ui_wifi_manager.h"
#include "app/app_data.h"
#include "net/wifi_provision.h"

namespace wiview {

void WifiManagerScreen::enter() {
    m_mode   = Mode::LIST;
    m_next   = ScreenId::WIFI_MGR;
    m_count  = WifiProvision::networkCount();
    m_cursor = WifiProvision::activeIndex();
    if (m_cursor < 0) m_cursor = 0;
    m_ssid   = "";
    m_pass   = "";
    m_dirty  = true;
    render();
}

void WifiManagerScreen::render() {
    auto& d = M5Cardputer.Display;
    d.clear(TFT_BLACK);
    d.setTextSize(2);
    d.setCursor(4, 4);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.println("WiFi");
    d.setTextSize(1);

    if (m_mode == Mode::LIST) {
        int active = WifiProvision::activeIndex();
        if (m_count == 0) {
            d.setCursor(4, 36);
            d.setTextColor(TFT_DARKGREY, TFT_BLACK);
            d.print("(no saved networks)");
        }
        for (int i = 0; i < m_count; i++) {
            String ssid, pass;
            WifiProvision::getNetwork(i, ssid, pass);
            int y = 30 + i * 12;
            d.setCursor(2, y);
            d.setTextColor(i == m_cursor ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
            d.printf("%c%c %s", (i == m_cursor ? '>' : ' '),
                     (i == active ? '*' : ' '), ssid.c_str());
        }
        d.setCursor(4, 122);
        d.setTextColor(TFT_DARKGREY, TFT_BLACK);
        d.print(";/. move  Enter:join a:add d:del");
    } else {
        bool pass_mode = (m_mode == Mode::ADD_PASS);
        d.setCursor(4, 36);
        d.setTextColor(TFT_CYAN, TFT_BLACK);
        d.print(pass_mode ? "Password:" : "New SSID:");
        d.setCursor(4, 52);
        d.setTextColor(TFT_GREEN, TFT_BLACK);
        if (pass_mode) {
            for (size_t i = 0; i < m_pass.length(); i++) d.print("*");
        } else {
            d.print(m_ssid.c_str());
        }
        d.setCursor(4, 110);
        d.setTextColor(TFT_DARKGREY, TFT_BLACK);
        d.print("Enter=next  Del=back  `=clear");
        d.setCursor(4, 122);
        d.print(pass_mode ? "saves + reboots to connect" : "G0=cancel");
    }
}

void WifiManagerScreen::update(const SensorData&) {
    // Static screen; redraws on key input.
}

void WifiManagerScreen::handleEntryKey(const Keyboard_Class::KeysState& keys,
                                       String& field, bool to_pass) {
    if (keys.enter) {
        if (!to_pass) {
            if (m_ssid.length() > 0) { m_mode = Mode::ADD_PASS; m_dirty = true; }
        } else {
            // Save (password may be empty for open networks) and reboot to join.
            int idx = WifiProvision::addNetwork(m_ssid, m_pass);
            if (idx >= 0) {
                WifiProvision::setActiveIndex(idx);
                delay(200);
                ESP.restart();
            } else {
                // Store full — bail back to the list.
                m_mode = Mode::LIST;
                m_dirty = true;
            }
        }
        return;
    }
    if (keys.del) {
        if (field.length() > 0) field.remove(field.length() - 1);
        m_dirty = true;
        return;
    }
    if (keys.space) {
        field += ' ';
        m_dirty = true;
        return;
    }
    for (char c : keys.word) {
        if (c == '`') { field = ""; m_dirty = true; }
        else if (c >= 32 && c <= 126) { field += c; m_dirty = true; }
    }
}

void WifiManagerScreen::handleListKey(const Keyboard_Class::KeysState& keys) {
    if (keys.enter) {
        if (m_count > 0) {
            WifiProvision::setActiveIndex(m_cursor);
            delay(200);
            ESP.restart();  // reconnect via the normal boot path
        }
        return;
    }
    for (char c : keys.word) {
        if (c == ';') {                              // up (arrow key)
            if (m_count > 0) m_cursor = (m_cursor + m_count - 1) % m_count;
            m_dirty = true;
        } else if (c == '.') {                       // down (arrow key)
            if (m_count > 0) m_cursor = (m_cursor + 1) % m_count;
            m_dirty = true;
        } else if (c == 'a') {                        // add a network
            m_mode = Mode::ADD_SSID;
            m_ssid = "";
            m_pass = "";
            m_dirty = true;
            return;
        } else if (c == 'd') {                        // delete selected
            if (m_count > 0) {
                WifiProvision::removeNetwork(m_cursor);
                m_count = WifiProvision::networkCount();
                if (m_cursor >= m_count) m_cursor = m_count > 0 ? m_count - 1 : 0;
                m_dirty = true;
            }
        }
    }
}

void WifiManagerScreen::handleKey(const Keyboard_Class::KeysState& keys) {
    switch (m_mode) {
        case Mode::LIST:     handleListKey(keys); break;
        case Mode::ADD_SSID: handleEntryKey(keys, m_ssid, false); break;
        case Mode::ADD_PASS: handleEntryKey(keys, m_pass, true); break;
    }
    if (m_dirty) {
        m_dirty = false;
        render();
    }
}

} // namespace wiview
