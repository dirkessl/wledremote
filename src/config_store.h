#pragma once

#include <Arduino.h>
#include <Preferences.h>

class ConfigStore {
public:
    void begin();

    // WiFi (managed by WiFiManager, but we track state)
    bool hasWiFiConfig();

    // WLED configuration
    bool hasWLEDConfig();
    String getWLEDHost();
    uint16_t getWLEDPort();
    void setWLEDHost(const String& host);
    void setWLEDPort(uint16_t port);
    void clearWLEDConfig();

    // Display settings
    uint8_t getBrightness();
    void setBrightness(uint8_t brightness);

    // HomeSpan pairing code
    String getHomeKitCode();
    void setHomeKitCode(const String& code);

    // Factory reset
    void factoryReset();

private:
    Preferences _prefs;
};

extern ConfigStore configStore;
