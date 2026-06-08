#include "homekit_bridge.h"
#include <math.h>

HomeKitBridge homeKitBridge;

// ---- Color conversion helpers ----

void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf, gf, bf;

    if (h < 60)       { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else              { rf = c; gf = 0; bf = x; }

    r = (uint8_t)((rf + m) * 255);
    g = (uint8_t)((gf + m) * 255);
    b = (uint8_t)((bf + m) * 255);
}

void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v) {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxVal = fmaxf(rf, fmaxf(gf, bf));
    float minVal = fminf(rf, fminf(gf, bf));
    float delta = maxVal - minVal;

    v = maxVal;
    s = (maxVal > 0) ? (delta / maxVal) : 0;

    if (delta == 0) {
        h = 0;
    } else if (maxVal == rf) {
        h = 60.0f * fmodf((gf - bf) / delta, 6.0f);
    } else if (maxVal == gf) {
        h = 60.0f * (((bf - rf) / delta) + 2.0f);
    } else {
        h = 60.0f * (((rf - gf) / delta) + 4.0f);
    }
    if (h < 0) h += 360.0f;
}

// ---- WLEDLight ----

WLEDLight::WLEDLight() : Service::LightBulb() {
    power = new Characteristic::On(0);
    brightness = new Characteristic::Brightness(100);
    brightness->setRange(0, 100, 1);
    hue = new Characteristic::Hue(0);
    hue->setRange(0, 360, 1);
    saturation = new Characteristic::Saturation(0);
    saturation->setRange(0, 100, 1);
}

boolean WLEDLight::update() {
    bool on = power->getNewVal();
    int bri = brightness->getNewVal();
    float h = hue->getNewVal();
    float s = saturation->getNewVal() / 100.0f;
    float v = bri / 100.0f;

    // Set power state
    wledClient.setPower(on);

    if (on) {
        // Set brightness (map 0-100 to 0-255)
        wledClient.setBrightness((uint8_t)(bri * 255 / 100));

        // Set color (HSV to RGB)
        uint8_t r, g, b_val;
        hsvToRgb(h, s, v, r, g, b_val);
        wledClient.setColor(r, g, b_val);
    }

    return true;
}

void WLEDLight::syncFromWLED(const WLEDState& state) {
    power->setVal(state.on ? 1 : 0);
    brightness->setVal((state.brightness * 100) / 255);

    float h, s, v;
    rgbToHsv(state.r, state.g, state.b, h, s, v);
    hue->setVal((int)h);
    saturation->setVal((int)(s * 100));
}

// ---- WLEDPresetSwitch ----

WLEDPresetSwitch::WLEDPresetSwitch(int id, const char* name)
    : Service::Switch(), presetId(id), presetName(name) {
    on = new Characteristic::On(false);
    new Characteristic::Name(name);
}

boolean WLEDPresetSwitch::update() {
    if (on->getNewVal()) {
        wledClient.setPreset(presetId);
        // Auto-turn off the switch after activation (it's momentary)
        on->setVal(0);
    }
    return true;
}

// ---- HomeKitBridge ----

void HomeKitBridge::begin(const char* setupCode) {
    if (_initialized) return;

    _setupCode = setupCode;
    homeSpan.setLogLevel(1);
    homeSpan.setStatusPin(2);  // Onboard LED
    homeSpan.setPairingCode(setupCode);
    homeSpan.setQRID(_setupID.c_str());
    homeSpan.begin(Category::Lighting, "WLED Bridge");

    // Main accessory: Light
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("WLED Light");
            new Characteristic::Manufacturer("Mittelpunkt Studios");
            new Characteristic::Model("WLED Remote");
            new Characteristic::FirmwareRevision("1.0.0");
        _light = new WLEDLight();

    // Add preset switches (up to 8)
    const auto& presets = wledClient.getPresets();
    int count = 0;
    for (const auto& preset : presets) {
        if (count >= 8) break;
        new SpanAccessory();
            new Service::AccessoryInformation();
                new Characteristic::Identify();
                new Characteristic::Name(preset.second.c_str());
            auto sw = new WLEDPresetSwitch(preset.first, preset.second.c_str());
            _presetSwitches.push_back(sw);
        count++;
    }

    _initialized = true;
    Serial.println("[HomeKit] Bridge started");
}

void HomeKitBridge::poll() {
    if (_initialized) {
        homeSpan.poll();
    }
}

void HomeKitBridge::syncState(const WLEDState& state) {
    if (_light && state.reachable) {
        _light->syncFromWLED(state);
    }
}

bool HomeKitBridge::isPaired() const {
    return _initialized;
}

String HomeKitBridge::getSetupURI() const {
    // Generate X-HM:// URI for HomeKit QR code
    // Payload: version(3) | reserved(4) | category(8) | flags(4) | setup_code(27) = 46 bits
    // Category 5 = Lighting, Flags 2 = IP pairing
    uint64_t code = 0;
    for (int i = 0; i < 8; i++) {
        if (_setupCode[i] >= '0' && _setupCode[i] <= '9') {
            code = code * 10 + (_setupCode[i] - '0');
        }
    }

    uint64_t payload = 0;
    payload |= (uint64_t)0 << 43;   // version = 0
    payload |= (uint64_t)0 << 39;   // reserved = 0
    payload |= (uint64_t)5 << 31;   // category = Lighting
    payload |= (uint64_t)2 << 27;   // flags = IP
    payload |= code;                 // setup code

    // Encode as 9-character base36
    static const char base36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char encoded[10];
    encoded[9] = '\0';
    for (int i = 8; i >= 0; i--) {
        encoded[i] = base36[payload % 36];
        payload /= 36;
    }

    return String("X-HM://00") + String(encoded) + _setupID;
}
