#pragma once
#include <M5Cardputer.h>
#include "app_data.h"
#include "ui/ui_screen.h"

namespace wiview {

/// Top-level application controller. Manages screen switching,
/// sensor data aggregation, and the main loop.
class AppController {
public:
    AppController();
    void begin();
    void update();

private:
    Screen* m_screens[3];   // HOME, WATERFALL, BREATHING
    ScreenId m_current;
    SensorData m_data;

    void initScreens();
    void collectData();
    void dispatchInput();
};

} // namespace wiview
