#include "wled_discovery.h"

WLEDDiscovery wledDiscovery;

bool WLEDDiscovery::begin() {
    if (!MDNS.begin("wled-remote")) {
        Serial.println("[mDNS] Failed to start");
        return false;
    }
    Serial.println("[mDNS] Started");
    return true;
}

int WLEDDiscovery::scan(uint32_t timeoutMs) {
    _devices.clear();

    Serial.println("[mDNS] Scanning for WLED devices...");
    int n = MDNS.queryService("wled", "tcp");

    if (n == 0) {
        Serial.println("[mDNS] No WLED devices found");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        WLEDDevice device;
        device.name = MDNS.hostname(i);
        device.ip = MDNS.address(i).toString();
        device.port = MDNS.port(i);
        _devices.push_back(device);
        Serial.printf("[mDNS] Found: %s at %s:%d\n",
                      device.name.c_str(), device.ip.c_str(), device.port);
    }

    return _devices.size();
}

const std::vector<WLEDDevice>& WLEDDiscovery::getDevices() const {
    return _devices;
}

const WLEDDevice* WLEDDiscovery::getDevice(int index) const {
    if (index < 0 || index >= (int)_devices.size()) return nullptr;
    return &_devices[index];
}

int WLEDDiscovery::getDeviceCount() const {
    return _devices.size();
}
