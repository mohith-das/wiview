#pragma once
#include <Arduino.h>

namespace wiview {

/// Manages WiFi credentials in NVS via Arduino Preferences.
/// Provides keyboard-based text entry when no saved creds exist.
class WifiProvision {
public:
    /// Load saved SSID/password from NVS. Returns true if both found.
    static bool loadFromNVS(String& ssid, String& password);

    /// Save SSID/password to NVS.
    static void saveToNVS(const String& ssid, const String& password);

    /// Clear saved credentials from NVS.
    static void clearNVS();
};

} // namespace wiview
