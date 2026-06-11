
#include "homekit_bridge.h"
#include <math.h>

namespace {
void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v) {
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float maxv = fmaxf(rf, fmaxf(gf, bf));
    float minv = fminf(rf, fminf(gf, bf));
    float d = maxv - minv;
    v = maxv;
    s = maxv <= 0.0f ? 0.0f : d / maxv;
    if (d <= 0.00001f) { h = 0.0f; return; }
    if (maxv == rf) h = 60.0f * fmodf(((gf - bf) / d), 6.0f);
    else if (maxv == gf) h = 60.0f * (((bf - rf) / d) + 2.0f);
    else h = 60.0f * (((rf - gf) / d) + 4.0f);
    if (h < 0.0f) h += 360.0f;
}

void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf = 0, gf = 0, bf = 0;
    if (h < 60.0f) { rf = c; gf = x; }
    else if (h < 120.0f) { rf = x; gf = c; }
    else if (h < 180.0f) { gf = c; bf = x; }
    else if (h < 240.0f) { gf = x; bf = c; }
    else if (h < 300.0f) { rf = x; bf = c; }
    else { rf = c; bf = x; }
    r = (uint8_t)roundf((rf + m) * 255.0f);
    g = (uint8_t)roundf((gf + m) * 255.0f);
    b = (uint8_t)roundf((bf + m) * 255.0f);
}
}

HomeKitBridge homeKitBridge;

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
    float s = saturation->getNewVal();

    wledClient.setPower(on);

    if (on) {
        uint8_t bri255 = (uint8_t)(bri * 255 / 100);
        wledClient.setBrightness(bri255);

        uint8_t r, g, b;
        hsvToRgb(h, s / 100.0f, bri / 100.0f, r, g, b);
        wledClient.setColor(r, g, b);
    }

    return true;
}

void WLEDLight::syncFromWLED(const WLEDState& state) {
    power->setVal(state.on ? 1 : 0);
    brightness->setVal((state.brightness * 100) / 255);

    float h, s, v;
    rgbToHsv(state.r, state.g, state.b, h, s, v);
    hue->setVal(h);
    saturation->setVal(s * 100.0f);
}

// ---- HomeKitBridge ----

void HomeKitBridge::begin(const char* setupCode) {

    if (_initialized) return;

    _setupCode = setupCode;
    homeSpan.setLogLevel(1);
    homeSpan.setStatusPin(2);
    homeSpan.setPairingCode(setupCode);
    homeSpan.setQRID(_setupID.c_str());

    //homeSpan.setWifiCredentials(ssid.c_str(), pass.c_str());

    Serial.printf("[HomeKit] Starting with STA IP %s",
                  WiFi.localIP().toString().c_str());
              
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
    return _initialized; // This just checks if begun, but for pairing state HomeSpan handles it internally.
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

void HomeKitBridge::reset() {
    homeSpan.processSerialCommand("H");
}
