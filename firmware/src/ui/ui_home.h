#pragma once
#include "ui_screen.h"

namespace wiview {

/// Home screen: presence status, motion bar, IMU gate, CSI health, calibration
class HomeScreen : public Screen {
public:
    void enter() override;
    void update(const SensorData& data) override;
    void handleKey(const Keyboard_Class::KeysState& keys) override;
    ScreenId nextScreen() override { return m_next; }

private:
    ScreenId m_next = ScreenId::HOME;
    void drawStatusBar(const SensorData& data);
};

} // namespace wiview
