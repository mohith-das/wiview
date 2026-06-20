#pragma once
#include "ui_screen.h"

namespace wiview {

/// Breathing waveform + BPM display
class BreathingScreen : public Screen {
public:
    void enter() override;
    void update(const SensorData& data) override;
    void handleKey(const Keyboard_Class::KeysState& keys) override;
    ScreenId nextScreen() override { return m_next; }

private:
    ScreenId m_next = ScreenId::BREATHING;
    static constexpr int WAVEFORM_Y = 24;
    static constexpr int WAVEFORM_H = 80;
    static constexpr int WAVEFORM_W = 180;
    int m_wave_idx = 0;
    float m_wave_history[180] = {};
};

} // namespace wiview
