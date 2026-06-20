#pragma once
#include "ui_screen.h"

namespace wiview {

/// CSI amplitude waterfall heatmap: subcarriers × time
class WaterfallScreen : public Screen {
public:
    void enter() override;
    void update(const SensorData& data) override;
    void handleKey(const Keyboard_Class::KeysState& keys) override;
    ScreenId nextScreen() override { return m_next; }

private:
    ScreenId m_next = ScreenId::WATERFALL;
    static constexpr int COL_W = 4;        // pixels per subcarrier column
    static constexpr int MAX_COLS = 52;    // max subcarriers to display
    static constexpr int GRID_Y = 18;      // top of waterfall area
    static constexpr int GRID_H = 100;     // height of waterfall area

    int m_scroll_y = GRID_Y;
};

} // namespace wiview
