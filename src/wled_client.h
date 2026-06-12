#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <atomic>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

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
public:
    void setHost(const String& host, uint16_t port = 80);

    bool fetchState();
    bool fetchEffects();
    bool fetchPresets();
    bool requestStateRefresh();
    void loadCache();
    bool asyncFetchAll();
    FetchStatus getFetchStatus() const { return _fetchStatus.load(); }
    void clearFetchStatus() { _fetchStatus.store(FetchStatus::IDLE); }
    bool isBusy() const { return _fetchStatus.load() == FetchStatus::IN_PROGRESS; }

    bool togglePower();
    bool setPower(bool on);
    bool setBrightness(uint8_t bri);
    bool setColor(uint8_t r, uint8_t g, uint8_t b);
    bool setState(bool on, uint8_t bri, uint8_t r, uint8_t g, uint8_t b);
    bool setPreset(int presetId);
    bool setEffect(int effectId);

    const WLEDState& getState() { return _state; }
    const std::vector<String>& getEffects() { return _effects; }
    const std::vector<std::pair<int, String>>& getPresets() { return _presets; }

    const String& getHost() const { return _host; }
    bool isConfigured() const { return _host.length() > 0; }
    bool isReachable() const { return _state.reachable; }

private:
    static constexpr size_t MAX_BODY_LEN = 384;
    enum class RequestKind : uint8_t {
        FETCH_STATE,
        FETCH_EFFECTS,
        FETCH_PRESETS,
        FETCH_ALL,
        SEND_STATE,
        SEND_POWER
    };

    struct Request {
        RequestKind kind;
        char body[MAX_BODY_LEN];
    };

    bool ensureWorker();
    bool enqueueRequest(RequestKind kind, const String* body = nullptr, bool dedupe = false, bool replaceStateCommand = false, bool prioritize = false);
    bool sendState(const String& json);
    bool parseStateResponse(const String& response, WLEDState& state);
    bool parseEffectsResponse(const String& response, std::vector<String>& effects);
    bool parsePresetsResponse(const String& response, std::vector<std::pair<int, String>>& presets, String* cacheJson = nullptr);
    bool httpGetInto(const String& path, uint32_t timeoutMs, String& response);
    bool httpPost(const String& path, const String& body);
    void processRequest(const Request& req);
    void markReachable(bool reachable);
    static void workerTaskEntry(void* parameter);

    String _host;
    uint16_t _port = 80;
    WLEDState _state;
    std::vector<String> _effects;
    std::vector<std::pair<int, String>> _presets;
    std::atomic<FetchStatus> _fetchStatus{FetchStatus::IDLE};
    SemaphoreHandle_t _dataMutex = nullptr;
    SemaphoreHandle_t _queueMutex = nullptr;
    QueueHandle_t _requestQueue = nullptr;
    TaskHandle_t _workerTask = nullptr;
    uint32_t _lastCommandMs = 0;
    uint32_t _lastStateRequestMs = 0;
    bool _queuedStateRefresh = false;
    bool _queuedEffectsRefresh = false;
    bool _queuedPresetsRefresh = false;
    bool _queuedFetchAll = false;
    bool _powerVerifyPending = false;
    bool _powerVerifyTargetOn = false;
};

extern WLEDClient wledClient;
