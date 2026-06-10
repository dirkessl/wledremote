#include "config_store.h"
#include <WiFi.h>

ConfigStore configStore;

void ConfigStore::begin() {
    _prefs.begin("wled-remote", false);
}

bool ConfigStore::hasWiFiConfig() {
    // WiFiManager stores credentials in its own NVS namespace
    // We just check if WiFi can connect (handled externally)
    return WiFi.SSID().length() > 0;
}

bool ConfigStore::hasWLEDConfig() {
    return _prefs.isKey("wled_host");
}

String ConfigStore::getWLEDHost() {
    if (!_prefs.isKey("wled_host")) return "";
    return _prefs.getString("wled_host", "");
}

uint16_t ConfigStore::getWLEDPort() {
    return _prefs.getUShort("wled_port", 80);
}

void ConfigStore::setWLEDHost(const String& host) {
    _prefs.putString("wled_host", host);
}

void ConfigStore::setWLEDPort(uint16_t port) {
    _prefs.putUShort("wled_port", port);
}

void ConfigStore::clearWLEDConfig() {
    _prefs.remove("wled_host");
    _prefs.remove("wled_port");
}

uint8_t ConfigStore::getBrightness() {
    return _prefs.getUChar("brightness", 128);
}

void ConfigStore::setBrightness(uint8_t brightness) {
    _prefs.putUChar("brightness", brightness);
}

String ConfigStore::getHomeKitCode() {
    if (!_prefs.isKey("hk_code")) return "46637726";
    return _prefs.getString("hk_code", "46637726");
}

void ConfigStore::setHomeKitCode(const String& code) {
    _prefs.putString("hk_code", code);
}

String ConfigStore::getHostname() {
    return _prefs.getString("hostname", "wled-remote");
}

void ConfigStore::setHostname(const String& hostname) {
    _prefs.putString("hostname", hostname);
}

String ConfigStore::getCachedPresets() {
    return _prefs.getString("cache_presets", "[]");
}

void ConfigStore::setCachedPresets(const String& json) {
    _prefs.putString("cache_presets", json);
}

String ConfigStore::getCachedEffects() {
    return _prefs.getString("cache_effects", "[]");
}

void ConfigStore::setCachedEffects(const String& json) {
    _prefs.putString("cache_effects", json);
}

void ConfigStore::factoryReset() {
    _prefs.clear();
}
