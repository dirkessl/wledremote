#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <atomic>

enum class FetchStatus {
    IDLE,
    IN_PROGRESS,
    DONE,
    FAILED
};

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
    friend void asyncFetchTask(void* parameter);
public:
    void setHost(const String& host, uint16_t port = 80);

    // Data refresh requests (non-blocking, launches FreeRTOS worker task)
    bool fetchState();
    bool fetchEffects();
    bool fetchPresets();
    bool requestStateRefresh();
    void loadCache();

    // Asynchronous fetch of state/effects/presets via FreeRTOS task
    // Returns: true if launched, false if already in progress
    bool asyncFetchAll();
    FetchStatus getFetchStatus() const { return _fetchStatus.load(); }
    void clearFetchStatus() { _fetchStatus.store(FetchStatus::IDLE); }

    // Control
    bool togglePower();
    bool setPower(bool on);
    bool setBrightness(uint8_t bri);
    bool setColor(uint8_t r, uint8_t g, uint8_t b);
    bool setState(bool on, uint8_t bri, uint8_t r, uint8_t g, uint8_t b);
    bool setPreset(int presetId);
    bool setEffect(int effectId);

    // Getters
    const WLEDState& getState() { return _state; }
    const std::vector<String>& getEffects() { return _effects; }
    const std::vector<std::pair<int, String>>& getPresets() { return _presets; }

    const String& getHost() const { return _host; }
    bool isConfigured() const { return _host.length() > 0; }
    bool isReachable() const { return _state.reachable; }

private:
    bool sendState(const String& json);
    bool startAsyncFetch(uint8_t flags, const String* postBody = nullptr);
    bool parseStateResponse(const String& response, WLEDState& state);
    bool parseEffectsResponse(const String& response, std::vector<String>& effects);
    bool parsePresetsResponse(const String& response, std::vector<std::pair<int, String>>& presets, String* cacheJson = nullptr);
    String httpGet(const String& path);
    bool httpPost(const String& path, const String& body);

    String _host;
    uint16_t _port = 80;
    WLEDState _state;
    std::vector<String> _effects;
    std::vector<std::pair<int, String>> _presets;    // id, name
    std::atomic<FetchStatus> _fetchStatus{FetchStatus::IDLE};
    uint8_t _fetchFlags = 0;
    String _pendingPostBody;
    SemaphoreHandle_t _dataMutex = nullptr;
};

extern WLEDClient wledClient;
