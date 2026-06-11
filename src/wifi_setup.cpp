#include "wifi_setup.h"
#include <WiFi.h>
#include <Preferences.h>
#include "config_server.h"
#include "config_store.h"

WiFiSetup wifiSetup;

static Preferences wifiPrefs;

// ---- Try saved credentials only (no portal) ----
bool WiFiSetup::begin(const char* apName, uint16_t portalTimeout) {
    wifiPrefs.begin("wifi-creds", true);
    String ssid = wifiPrefs.getString("ssid", "");
    String pass = wifiPrefs.getString("pass", "");
    wifiPrefs.end();

    if (ssid.length() > 0) {
        Serial.printf("[WiFi] Trying saved SSID: %s\n", ssid.c_str());
        if (tryConnectSaved(ssid, pass)) return true;
        Serial.println("[WiFi] Saved credentials failed");
    } else {
        Serial.println("[WiFi] No saved credentials");
    }

    return false;
}

// ---- Non-blocking captive portal ----
void WiFiSetup::startPortal(const char* apName) {
    // Clean slate
    WiFi.disconnect(true);
    delay(200);

    _wm.setDebugOutput(true, "WM: ");
    _wm.setConfigPortalTimeout(180);
    _wm.setAPStaticIPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    _wm.setConfigPortalBlocking(false);   // KEY: non-blocking
    _wm.startConfigPortal(apName);

    Serial.println("[WiFi] Portal started non-blocking — call processPortal() in loop");
}

bool WiFiSetup::processPortal() {
    _wm.process();   // Handle DNS + HTTP requests — must be called every loop()

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Portal: connected!");
        saveCredentials();
        return true;
    }
    return false;
}

bool WiFiSetup::isPortalActive() {
    return _wm.getConfigPortalActive();
}

void WiFiSetup::saveCredentials() {
    delay(500);
    String newSSID = _wm.getWiFiSSID();
    String newPass = _wm.getWiFiPass();
    if (newSSID.length() == 0) { newSSID = WiFi.SSID(); newPass = WiFi.psk(); }
    if (newSSID.length() > 0) {
        wifiPrefs.begin("wifi-creds", false);
        wifiPrefs.putString("ssid", newSSID);
        wifiPrefs.putString("pass", newPass);
        wifiPrefs.end();
        Serial.printf("[WiFi] Credentials saved for: %s\n", newSSID.c_str());
    }
}

// ---- Non-blocking reconnect ----
static bool _isReconnecting = false;
static uint32_t _reconnectStart = 0;

bool WiFiSetup::reconnect(uint8_t maxAttempts) {
    if (WiFi.status() == WL_CONNECTED) {
        _isReconnecting = false;
        return true;
    }

    if (_isReconnecting) {
        if (millis() - _reconnectStart > 20000) {
            Serial.println("[WiFi] Reconnect timeout");
            _isReconnecting = false;
        }
        return false;
    }

    wifiPrefs.begin("wifi-creds", true);
    String ssid = wifiPrefs.getString("ssid", "");
    String pass = wifiPrefs.getString("pass", "");
    wifiPrefs.end();

    if (ssid.length() == 0) return false;

    Serial.printf("[WiFi] Starting non-blocking reconnect to: %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.disconnect(false, false);
    WiFi.setHostname(configStore.getHostname().c_str());

    delay(100);
    WiFi.begin(ssid.c_str(), pass.c_str());
                                    
    _isReconnecting = true;
    _reconnectStart = millis();

    return false;
}

// ---- Try connecting with saved creds (blocking, used at boot only) ----
bool WiFiSetup::tryConnectSaved(const String& ssid, const String& pass) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    for (int attempt = 1; attempt <= 2; attempt++) {
        Serial.printf("[WiFi] Connection attempt %d/2\n", attempt);
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
            delay(250);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            
            return true;
        }

        Serial.printf("[WiFi] Attempt %d failed\n", attempt);
        WiFi.disconnect(true);
        delay(500);
    }

    return false;
}

bool WiFiSetup::hasSavedCredentials() {
    Preferences prefs;
    prefs.begin("wifi-creds", true);
    String ssid = prefs.getString("ssid", "");
    prefs.end();
    return ssid.length() > 0;
}

void WiFiSetup::resetCredentials() {
    _wm.resetSettings();
    wifiPrefs.begin("wifi-creds", false);
    wifiPrefs.clear();
    wifiPrefs.end();
}

String WiFiSetup::getIPAddress() {
    return WiFi.localIP().toString();
}

String WiFiSetup::getSavedSSID() {
    Preferences prefs;
    prefs.begin("wifi-creds", true);
    String ssid = prefs.getString("ssid", "");
    prefs.end();
    return ssid;
}

String WiFiSetup::getSavedPassword() {
    Preferences prefs;
    prefs.begin("wifi-creds", true);
    String pass = prefs.getString("pass", "");
    prefs.end();
    return pass;
}


bool WiFiSetup::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
