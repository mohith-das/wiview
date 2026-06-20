#pragma once
#include <M5Cardputer.h>
#include <M5Unified.h>

namespace wiview {

/// Screen identifiers. The first NUM_MAIN_SCREENS are the views G0 cycles
/// through; screens after that are config/modal screens reached by hotkey.
enum class ScreenId {
    HOME,
    WATERFALL,
    BREATHING,
    HOST_SETUP,
    WIFI_MGR,
    COUNT
};

static constexpr int NUM_MAIN_SCREENS = 3;  // HOME, WATERFALL, BREATHING

/// Base class for all display screens
class Screen {
public:
    virtual ~Screen() = default;
    virtual void enter() = 0;           // Called when screen becomes active
    virtual void update(const struct SensorData& data) = 0;  // Redraw if needed
    virtual void handleKey(const Keyboard_Class::KeysState& keys) = 0;
    virtual ScreenId nextScreen() = 0;  // Non-zero to switch; zero = stay
protected:
    // Helper: bound a float to display width
    static int clamp_bar(int val, int max) { return val < 0 ? 0 : (val > max ? max : val); }
};

} // namespace wiview
