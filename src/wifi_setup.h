#pragma once

#include <Arduino.h>
#include <WiFiManager.h>

class WiFiSetup {
public:
    bool begin(const char* apName = "WLED-Bridge", uint16_t portalTimeout = 180);

    // Start captive portal in non-blocking mode (call processPortal() in loop)
    void startPortal(const char* apName = "WLED-Bridge");

    String getSavedSSID();
    String getSavedPassword();

    // Call every loop() while portal is active.
    // Returns true if WiFi just connected via portal.
    // Returns false if still waiting.
    bool processPortal();

    // Returns true while the portal AP is still active
    bool isPortalActive();

    // Check if we have stored WiFi credentials
    bool hasSavedCredentials();

    // Reset saved WiFi credentials
    void resetCredentials();

    // Get current IP as string
    String getIPAddress();

    // Try to reconnect using saved credentials (non-blocking state machine)
    bool reconnect(uint8_t maxAttempts = 3);

    // Check connection status
    bool isConnected();

private:
    bool tryConnectSaved(const String& ssid, const String& pass);
    void saveCredentials();
    WiFiManager _wm;
};

extern WiFiSetup wifiSetup;
