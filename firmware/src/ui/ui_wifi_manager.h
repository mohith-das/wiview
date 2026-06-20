#pragma once
#include "ui_screen.h"

namespace wiview {

/// WiFi manager: list saved networks, switch between them, add, and delete.
/// Opened with the 'w' key.
///
///   List mode:  ; = up, . = down, Enter = connect (reboot), a = add,
///               d = delete, G0 = back.
///   Add mode:   type SSID -> Enter -> type password -> Enter (saves, reboots).
class WifiManagerScreen : public Screen {
public:
    void enter() override;
    void update(const SensorData& data) override;
    void handleKey(const Keyboard_Class::KeysState& keys) override;
    ScreenId nextScreen() override { return m_next; }

private:
    enum class Mode { LIST, ADD_SSID, ADD_PASS };

    void render();
    void handleListKey(const Keyboard_Class::KeysState& keys);
    void handleEntryKey(const Keyboard_Class::KeysState& keys, String& field, bool to_pass);

    Mode     m_mode = Mode::LIST;
    ScreenId m_next = ScreenId::WIFI_MGR;
    int      m_count = 0;
    int      m_cursor = 0;
    String   m_ssid;
    String   m_pass;
    bool     m_dirty = true;
};

} // namespace wiview
