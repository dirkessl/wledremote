#pragma once

#include <Arduino.h>
#include "HomeSpan.h"
#include "wled_client.h"

// HSV to RGB conversion helper
void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b);
void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v);

struct WLEDLight : Service::LightBulb {
    SpanCharacteristic* power;
    SpanCharacteristic* brightness;
    SpanCharacteristic* hue;
    SpanCharacteristic* saturation;

    WLEDLight();
    boolean update() override;
    void syncFromWLED(const WLEDState& state);
};

struct WLEDPresetSwitch : Service::Switch {
    SpanCharacteristic* on;
    int presetId;
    String presetName;

    WLEDPresetSwitch(int id, const char* name);
    boolean update() override;
};

class HomeKitBridge {
public:
    void begin(const char* setupCode = "46637726");
    void poll();
    void syncState(const WLEDState& state);
    bool isPaired() const;
    String getSetupURI() const;

private:
    WLEDLight* _light = nullptr;
    std::vector<WLEDPresetSwitch*> _presetSwitches;
    bool _initialized = false;
    String _setupCode = "46637726";
    String _setupID = "WLED";
};

extern HomeKitBridge homeKitBridge;
