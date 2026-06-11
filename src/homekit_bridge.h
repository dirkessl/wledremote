#pragma once

#include <Arduino.h>
#include "HomeSpan.h"
#include "wled_client.h"

struct WLEDLight : Service::LightBulb {
    SpanCharacteristic* power;
    SpanCharacteristic* brightness;
    SpanCharacteristic* hue;
    SpanCharacteristic* saturation;

    WLEDLight();
    boolean update() override;
    void syncFromWLED(const WLEDState& state);
};

class HomeKitBridge {
public:
    void begin(const char* setupCode = "46637726");
    void poll();
    void syncState(const WLEDState& state);
    bool isPaired() const;
    String getSetupURI() const;
    void reset();

private:
    WLEDLight* _light = nullptr;
    bool _initialized = false;
    String _setupCode = "46637726";
    String _setupID = "WLED";
};

extern HomeKitBridge homeKitBridge;
