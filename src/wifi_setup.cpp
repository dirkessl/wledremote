#include "wifi_setup.h"
#include <WiFi.h>
#include <Preferences.h>

WiFiSetup wifiSetup;

static Preferences wifiPrefs;

bool WiFiSetup::begin(const char* apName, uint16_t portalTimeout) {
    // Enable verbose WiFi debug output
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_START:
                Serial.println("[WiFi] Station started");
                break;
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Serial.println("[WiFi] Connected to AP");
                break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Serial.printf("[WiFi] Got IP: %s\n", WiFi.localIP().toString().c_str());
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Serial.printf("[WiFi] Disconnected, reason: %d\n", info.wifi_sta_disconnected.reason);
                break;
            case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
                Serial.println("[WiFi] Auth mode changed");
                break;
            default:
                Serial.printf("[WiFi] Event: %d\n", event);
                break;
        }
    });

    // Try to connect with our own saved credentials first
    wifiPrefs.begin("wifi-creds", true);  // read-only
    String ssid = wifiPrefs.getString("ssid", "");
    String pass = wifiPrefs.getString("pass", "");
    wifiPrefs.end();

     if (ssid.length() > 0) {
         Serial.printf("[WiFi] Trying saved SSID: %s (pass len: %d)\n", ssid.c_str(), pass.length());

         // Try connection with saved credentials
        if (tryConnectSaved(ssid, pass)) {
            return true;
         }

        Serial.println("[WiFi] Saved credentials failed, starting portal");
     } else {
        Serial.println("[WiFi] No saved credentials found");
    }

    // Start WiFiManager portal directly (skip autoConnect's internal
    // connection attempt since it has no stored creds on Core 3.x)
    _wm.setDebugOutput(true, "WM: ");
    _wm.setConfigPortalTimeout(portalTimeout);
    _wm.setConnectTimeout(60);

    bool connected = _wm.startConfigPortal(apName);

    // If connected, save the credentials ourselves
    if (connected) {
        // Wait a moment for WiFi stack to stabilize
        delay(1000);
        
        // Try WiFiManager getters first, fall back to WiFi class
        String newSSID = _wm.getWiFiSSID();
        String newPass = _wm.getWiFiPass();
        
        if (newSSID.length() == 0) {
            newSSID = WiFi.SSID();
            newPass = WiFi.psk();
        }
        
        Serial.printf("[WiFi] Saving credentials - SSID: '%s', pass length: %d\n", 
                      newSSID.c_str(), newPass.length());
        
        if (newSSID.length() > 0) {
            wifiPrefs.begin("wifi-creds", false);
            wifiPrefs.putString("ssid", newSSID);
            wifiPrefs.putString("pass", newPass);
            wifiPrefs.end();
            Serial.printf("[WiFi] Credentials saved for: %s\n", newSSID.c_str());
        } else {
            Serial.println("[WiFi] WARNING: Could not get SSID to save!");
        }
    }

     return connected;
   }

// Reconnect using saved credentials (called from main loop after detecting disconnect)
bool WiFiSetup::reconnect(uint8_t maxAttempts) {
    wifiPrefs.begin("wifi-creds", true);
    String ssid = wifiPrefs.getString("ssid", "");
    String pass = wifiPrefs.getString("pass", "");
    wifiPrefs.end();

    if (ssid.length() == 0) return false;

    Serial.printf("[WiFi] Reconnect attempt (max %d), SSID: %s\n", maxAttempts, ssid.c_str());
    bool connected = tryConnectSaved(ssid, pass);
    if (!connected) {
        Serial.println("[WiFi] Reconnect failed");
       }
    return connected;
   }

// Try connecting with given credentials (up to 2 attempts per call)
bool WiFiSetup::tryConnectSaved(const String& ssid, const String& pass) {
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);

    for (int attempt = 1; attempt <= 2; attempt++) {
        Serial.printf("[WiFi] Connection attempt %d/2\n", attempt);
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
            delay(250);
            Serial.print(".");
           }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[WiFi] Connected with saved credentials");
            return true;
           }

        Serial.printf("[WiFi] Attempt %d failed, status: %d\n", attempt, WiFi.status());
        WiFi.disconnect(true);
        delay(1000);
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

bool WiFiSetup::startPortal(const char* apName) {
    _wm.setConfigPortalTimeout(180);
    return _wm.startConfigPortal(apName);
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

bool WiFiSetup::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
