
#include "homekit_bridge.h"

HomeKitBridge homeKitBridge;

// ---- WLEDLight ----

WLEDLight::WLEDLight() : Service::LightBulb() {
    power = new Characteristic::On(0);
    brightness = new Characteristic::Brightness(100);
    brightness->setRange(0, 100, 1);
}

boolean WLEDLight::update() {
    bool on = power->getNewVal();
    int bri = brightness->getNewVal();

    // Set power state
    wledClient.setPower(on);

    if (on) {
        // Set brightness (map 0-100 to 0-255)
        wledClient.setBrightness((uint8_t)(bri * 255 / 100));
    }

    return true;
}

void WLEDLight::syncFromWLED(const WLEDState& state) {
    power->setVal(state.on ? 1 : 0);
    brightness->setVal((state.brightness * 100) / 255);
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
