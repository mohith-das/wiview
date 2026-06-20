#pragma once
#include <M5Cardputer.h>
#include "app_data.h"
#include "ui/ui_screen.h"
#include "ui/ui_wifi_setup.h"

namespace wiview {

class AppController {
public:
    AppController();
    void begin();
    void update();

private:
    Screen* m_screens[3];
    WifiSetupScreen* m_screensWifi = nullptr;
    ScreenId m_current;
    SensorData m_data;

    void initScreens();
    void finishInit();
    void showWifiSetup();
    void collectData();
    void dispatchInput();
};

} // namespace wiview
