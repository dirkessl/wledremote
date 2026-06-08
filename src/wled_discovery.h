#pragma once

#include <Arduino.h>
#include <ESPmDNS.h>
#include <vector>

struct WLEDDevice {
    String name;
    String ip;
    uint16_t port;
};

class WLEDDiscovery {
public:
    // Initialize mDNS (call after WiFi connected)
    bool begin();

    // Scan for WLED devices on the network
    // Returns number of devices found
    int scan(uint32_t timeoutMs = 3000);

    // Get discovered devices
    const std::vector<WLEDDevice>& getDevices() const;

    // Get device by index
    const WLEDDevice* getDevice(int index) const;

    // Get device count
    int getDeviceCount() const;

private:
    std::vector<WLEDDevice> _devices;
};

extern WLEDDiscovery wledDiscovery;
