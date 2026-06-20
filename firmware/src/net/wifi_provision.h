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

    /// Remove only the WiFi credentials (keeps host IP / RuView-mode settings).
    static void clearWifiCreds();

    /// Load the saved stream-host IP (e.g. "192.168.1.50"). Empty if none.
    static String loadHostFromNVS();

    /// Save the stream-host IP (discovered or manually entered).
    static void saveHostToNVS(const String& host);

    /// Load/save the "RuView direct" output-format toggle.
    static bool loadRuViewModeFromNVS();
    static void saveRuViewModeToNVS(bool on);
};

} // namespace wiview
