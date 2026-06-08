#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct WLEDState {
    bool on = false;
    uint8_t brightness = 0;
    uint8_t r = 0, g = 0, b = 0;
    int preset = -1;
    int effect = 0;
    int palette = 0;
    String effectName;
    bool reachable = false;
};

class WLEDClient {
public:
    void setHost(const String& host, uint16_t port = 80);

    // State queries
    bool fetchState();
    bool fetchEffects();
    bool fetchPresets();

    // Control
    bool togglePower();
    bool setPower(bool on);
    bool setBrightness(uint8_t bri);
    bool setColor(uint8_t r, uint8_t g, uint8_t b);
    bool setPreset(int presetId);
    bool setEffect(int effectId);

    // Getters
    const WLEDState& getState() const { return _state; }
    const std::vector<String>& getEffects() const { return _effects; }
    const std::vector<std::pair<int, String>>& getPresets() const { return _presets; }

    bool isConfigured() const { return _host.length() > 0; }
    bool isReachable() const { return _state.reachable; }

private:
    bool sendState(const String& json);
    String httpGet(const String& path);
    bool httpPost(const String& path, const String& body);

    String _host;
    uint16_t _port = 80;
    WLEDState _state;
    std::vector<String> _effects;
    std::vector<std::pair<int, String>> _presets;  // id, name
};

extern WLEDClient wledClient;
