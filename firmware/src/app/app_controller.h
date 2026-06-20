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
    Screen* m_screens[(int)ScreenId::COUNT];
    WifiSetupScreen* m_screensWifi = nullptr;
    ScreenId m_current;
    SensorData m_data;
    uint32_t m_wifiForgetArmedMs = 0;  // timestamp of first 'w' press (0 = not armed)

    void initScreens();
    void finishInit();
    void showWifiSetup();
    void collectData();
    void dispatchInput();
    void pollDiscovery();
    void setStreamHost(const IPAddress& host, uint16_t port);
};

} // namespace wiview
