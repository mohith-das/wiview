#include "wifi_provision.h"
#include <Preferences.h>

namespace wiview {

static constexpr const char* NVS_NS = "wiview";
static constexpr const char* KEY_SSID = "ssid";     // legacy single-network keys
static constexpr const char* KEY_PASS = "pass";
static constexpr const char* KEY_HOST = "host";
static constexpr const char* KEY_RVMODE = "rvmode";
static constexpr const char* KEY_WIFI_N = "wifi_n";   // number of saved networks
static constexpr const char* KEY_WIFI_ACT = "wifi_act";

// Per-network keys: "ssid<i>" / "pass<i>" (i = 0..MAX_NETWORKS-1).
static String ssidKey(int i) { return String("ssid") + i; }
static String passKey(int i) { return String("pass") + i; }

// One-time migration of the legacy single ssid/pass into the network list.
static void migrate() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    if (!prefs.isKey(KEY_WIFI_N)) {
        String s = prefs.getString(KEY_SSID, "");
        String p = prefs.getString(KEY_PASS, "");
        if (s.length() > 0) {
            prefs.putString(ssidKey(0).c_str(), s);
            prefs.putString(passKey(0).c_str(), p);
            prefs.putUChar(KEY_WIFI_N, 1);
            prefs.putUChar(KEY_WIFI_ACT, 0);
        } else {
            prefs.putUChar(KEY_WIFI_N, 0);
        }
    }
    prefs.end();
}

int WifiProvision::networkCount() {
    migrate();
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return 0;
    int n = prefs.getUChar(KEY_WIFI_N, 0);
    prefs.end();
    if (n < 0) n = 0;
    if (n > MAX_NETWORKS) n = MAX_NETWORKS;
    return n;
}

bool WifiProvision::getNetwork(int i, String& ssid, String& password) {
    if (i < 0 || i >= networkCount()) return false;
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return false;
    ssid     = prefs.getString(ssidKey(i).c_str(), "");
    password = prefs.getString(passKey(i).c_str(), "");
    prefs.end();
    return ssid.length() > 0;
}

int WifiProvision::activeIndex() {
    int n = networkCount();
    if (n == 0) return -1;
    Preferences prefs;
    prefs.begin(NVS_NS, true);
    int a = prefs.getUChar(KEY_WIFI_ACT, 0);
    prefs.end();
    if (a < 0 || a >= n) a = 0;
    return a;
}

void WifiProvision::setActiveIndex(int i) {
    int n = networkCount();
    if (n == 0 || i < 0 || i >= n) return;
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putUChar(KEY_WIFI_ACT, (uint8_t)i);
    prefs.end();
}

int WifiProvision::addNetwork(const String& ssid, const String& password) {
    if (ssid.length() == 0) return -1;
    int n = networkCount();

    // Update password if this SSID is already saved.
    for (int i = 0; i < n; i++) {
        String s, p;
        getNetwork(i, s, p);
        if (s == ssid) {
            Preferences prefs;
            prefs.begin(NVS_NS, false);
            prefs.putString(passKey(i).c_str(), password);
            prefs.end();
            return i;
        }
    }

    if (n >= MAX_NETWORKS) return -1;
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(ssidKey(n).c_str(), ssid);
    prefs.putString(passKey(n).c_str(), password);
    prefs.putUChar(KEY_WIFI_N, (uint8_t)(n + 1));
    prefs.end();
    return n;
}

void WifiProvision::removeNetwork(int i) {
    int n = networkCount();
    if (i < 0 || i >= n) return;

    Preferences prefs;
    prefs.begin(NVS_NS, false);
    // Shift later entries down into the gap.
    for (int j = i; j < n - 1; j++) {
        prefs.putString(ssidKey(j).c_str(), prefs.getString(ssidKey(j + 1).c_str(), ""));
        prefs.putString(passKey(j).c_str(), prefs.getString(passKey(j + 1).c_str(), ""));
    }
    prefs.remove(ssidKey(n - 1).c_str());
    prefs.remove(passKey(n - 1).c_str());
    prefs.putUChar(KEY_WIFI_N, (uint8_t)(n - 1));

    // Keep the active index pointing at a valid (and, where possible, the same) net.
    int a = prefs.getUChar(KEY_WIFI_ACT, 0);
    if (a == i)      a = 0;
    else if (a > i)  a -= 1;
    if (a < 0 || a >= n - 1) a = 0;
    prefs.putUChar(KEY_WIFI_ACT, (uint8_t)a);
    prefs.end();
}

// ── Legacy/active-network compatibility API ────────────────────────────────

bool WifiProvision::loadFromNVS(String& ssid, String& password) {
    int a = activeIndex();
    if (a < 0) return false;
    return getNetwork(a, ssid, password);
}

void WifiProvision::saveToNVS(const String& ssid, const String& password) {
    int idx = addNetwork(ssid, password);
    if (idx >= 0) setActiveIndex(idx);
}

void WifiProvision::clearNVS() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.clear();
    prefs.end();
}

void WifiProvision::clearWifiCreds() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    int n = prefs.getUChar(KEY_WIFI_N, 0);
    for (int i = 0; i < n; i++) {
        prefs.remove(ssidKey(i).c_str());
        prefs.remove(passKey(i).c_str());
    }
    prefs.putUChar(KEY_WIFI_N, 0);
    prefs.remove(KEY_WIFI_ACT);
    prefs.remove(KEY_SSID);
    prefs.remove(KEY_PASS);
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
