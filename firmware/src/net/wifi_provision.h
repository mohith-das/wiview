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

    // ── Multi-network store ────────────────────────────────────────────────
    static constexpr int MAX_NETWORKS = 8;

    /// Number of saved networks (0..MAX_NETWORKS). Migrates legacy creds.
    static int  networkCount();
    /// Read network i (0-based). Returns false if i is out of range.
    static bool getNetwork(int i, String& ssid, String& password);
    /// Index of the network connected on boot (clamped to a valid entry).
    static int  activeIndex();
    static void setActiveIndex(int i);
    /// Add (or update password of an existing) network. Returns its index, or
    /// -1 if the store is full.
    static int  addNetwork(const String& ssid, const String& password);
    /// Remove network i; later entries shift down. Adjusts the active index.
    static void removeNetwork(int i);
};

} // namespace wiview
