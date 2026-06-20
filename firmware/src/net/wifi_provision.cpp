#include "wifi_provision.h"
#include <Preferences.h>

namespace wiview {

static constexpr const char* NVS_NS = "wiview";
static constexpr const char* KEY_SSID = "ssid";
static constexpr const char* KEY_PASS = "pass";
static constexpr const char* KEY_HOST = "host";
static constexpr const char* KEY_RVMODE = "rvmode";

bool WifiProvision::loadFromNVS(String& ssid, String& password) {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return false;
    ssid     = prefs.getString(KEY_SSID, "");
    password = prefs.getString(KEY_PASS, "");
    prefs.end();
    return (ssid.length() > 0 && password.length() > 0);
}

void WifiProvision::saveToNVS(const String& ssid, const String& password) {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(KEY_SSID, ssid);
    prefs.putString(KEY_PASS, password);
    prefs.end();
}

void WifiProvision::clearNVS() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.clear();
    prefs.end();
}

String WifiProvision::loadHostFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return "";
    String host = prefs.getString(KEY_HOST, "");
    prefs.end();
    return host;
}

void WifiProvision::saveHostToNVS(const String& host) {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(KEY_HOST, host);
    prefs.end();
}

bool WifiProvision::loadRuViewModeFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return false;
    bool on = prefs.getUChar(KEY_RVMODE, 0) != 0;
    prefs.end();
    return on;
}

void WifiProvision::saveRuViewModeToNVS(bool on) {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putUChar(KEY_RVMODE, on ? 1 : 0);
    prefs.end();
}

} // namespace wiview
