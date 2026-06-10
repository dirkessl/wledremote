#pragma once

#include <Arduino.h>
#include <WiFiManager.h>

class WiFiSetup {
public:
    // Returns true if WiFi connected, false if portal timed out
    bool begin(const char* apName = "WLED-Bridge", uint16_t portalTimeout = 180);

    // Force start config portal (e.g., from menu)
    bool startPortal(const char* apName = "WLED-Bridge");

    // Check if we have stored WiFi credentials
    bool hasSavedCredentials();

    // Reset saved WiFi credentials
    void resetCredentials();

    // Get current IP as string
    String getIPAddress();

     // Try to reconnect using saved credentials (non-portal)
     // Returns true if reconnected, false after exhausting attempts
    bool reconnect(uint8_t maxAttempts = 3);

    // Check connection status
    bool isConnected();

private:
    bool tryConnectSaved(const String& ssid, const String& pass);
    WiFiManager _wm;
};

extern WiFiSetup wifiSetup;
