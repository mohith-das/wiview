#pragma once
#include <M5Cardputer.h>
#include <M5Unified.h>

namespace wiview {

/// Screen identifiers
enum class ScreenId {
    HOME,
    WATERFALL,
    BREATHING,
    COUNT
};

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
