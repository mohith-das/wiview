#include "wifi_provision.h"
#include <Preferences.h>

namespace wiview {

static constexpr const char* NVS_NS = "wiview";
static constexpr const char* KEY_SSID = "ssid";
static constexpr const char* KEY_PASS = "pass";

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

} // namespace wiview
