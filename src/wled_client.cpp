#include "wled_client.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include "config_store.h"

namespace {
constexpr uint32_t STATE_REFRESH_MIN_MS = 2500;
constexpr uint32_t POST_REFRESH_GUARD_MS = 800;
constexpr UBaseType_t REQUEST_QUEUE_LENGTH = 8;
}

WLEDClient wledClient;

void WLEDClient::workerTaskEntry(void* arg) {
    WLEDClient* client = static_cast<WLEDClient*>(arg);
    Request req{};
    while (true) {
        if (xQueueReceive(client->_requestQueue, &req, portMAX_DELAY) == pdTRUE) {
            client->_fetchStatus.store(FetchStatus::IN_PROGRESS);
            client->processRequest(req);
        }
    }
}

void WLEDClient::loadCache() {
    String effJson = configStore.getCachedEffects();
    if (effJson.length() > 2) {
        JsonDocument doc;
        if (!deserializeJson(doc, effJson)) {
            _effects.clear();
            for (JsonVariant v : doc.as<JsonArray>()) {
                _effects.push_back(v.as<String>());
            }
        }
    }

    String preJson = configStore.getCachedPresets();
    if (preJson.length() > 2) {
        JsonDocument doc;
        if (!deserializeJson(doc, preJson)) {
            _presets.clear();
            for (JsonVariant v : doc.as<JsonArray>()) {
                _presets.push_back({v["id"].as<int>(), v["n"].as<String>()});
            }
        }
    }
}

void WLEDClient::setHost(const String& host, uint16_t port) {
    _host = host;
    _port = port;
}

bool WLEDClient::ensureWorker() {
    if (!_dataMutex) {
        _dataMutex = xSemaphoreCreateRecursiveMutex();
        if (!_dataMutex) return false;
    }
    if (!_queueMutex) {
        _queueMutex = xSemaphoreCreateMutex();
        if (!_queueMutex) return false;
    }
    if (!_requestQueue) {
        _requestQueue = xQueueCreate(REQUEST_QUEUE_LENGTH, sizeof(Request));
        if (!_requestQueue) return false;
    }
    if (!_workerTask) {
        BaseType_t ok = xTaskCreate(workerTaskEntry, "wled_worker", 8192, this, 1, &_workerTask);
        if (ok != pdPASS) {
            _workerTask = nullptr;
            return false;
        }
    }
    return true;
}

bool WLEDClient::enqueueRequest(RequestKind kind, const String* body, bool dedupe, bool replaceStateCommand) {
    if (!ensureWorker()) {
        _fetchStatus.store(FetchStatus::FAILED);
        return false;
    }

    Request req{};
    req.kind = kind;
    req.body[0] = '\0';
    if (body) {
        size_t len = std::min(body->length(), MAX_BODY_LEN - 1);
        memcpy(req.body, body->c_str(), len);
        req.body[len] = '\0';
    }

    xSemaphoreTake(_queueMutex, portMAX_DELAY);

    bool* dedupeFlag = nullptr;
    switch (kind) {
        case RequestKind::FETCH_STATE: dedupeFlag = &_queuedStateRefresh; break;
        case RequestKind::FETCH_EFFECTS: dedupeFlag = &_queuedEffectsRefresh; break;
        case RequestKind::FETCH_PRESETS: dedupeFlag = &_queuedPresetsRefresh; break;
        case RequestKind::FETCH_ALL: dedupeFlag = &_queuedFetchAll; break;
        default: break;
    }

    if (dedupe && dedupeFlag && *dedupeFlag) {
        xSemaphoreGive(_queueMutex);
        return false;
    }

    if (replaceStateCommand) {
        UBaseType_t waiting = uxQueueMessagesWaiting(_requestQueue);
        if (waiting > 0) {
            Request temp[REQUEST_QUEUE_LENGTH];
            UBaseType_t kept = 0;
            for (UBaseType_t i = 0; i < waiting; ++i) {
                Request cur{};
                if (xQueueReceive(_requestQueue, &cur, 0) == pdTRUE) {
                    if (cur.kind == RequestKind::SEND_STATE) continue;
                    temp[kept++] = cur;
                }
            }
            xQueueReset(_requestQueue);
            _queuedStateRefresh = false;
            _queuedEffectsRefresh = false;
            _queuedPresetsRefresh = false;
            _queuedFetchAll = false;
            for (UBaseType_t i = 0; i < kept; ++i) {
                xQueueSend(_requestQueue, &temp[i], 0);
                switch (temp[i].kind) {
                    case RequestKind::FETCH_STATE: _queuedStateRefresh = true; break;
                    case RequestKind::FETCH_EFFECTS: _queuedEffectsRefresh = true; break;
                    case RequestKind::FETCH_PRESETS: _queuedPresetsRefresh = true; break;
                    case RequestKind::FETCH_ALL: _queuedFetchAll = true; break;
                    default: break;
                }
            }
        }
    }

    if (xQueueSend(_requestQueue, &req, 0) != pdTRUE) {
        xSemaphoreGive(_queueMutex);
        _fetchStatus.store(FetchStatus::FAILED);
        return false;
    }

    if (dedupeFlag) *dedupeFlag = true;
    if (kind == RequestKind::SEND_STATE) _lastCommandMs = millis();
    xSemaphoreGive(_queueMutex);
    return true;
}

bool WLEDClient::parseStateResponse(const String& response, WLEDState& state) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.printf("[WLED] JSON parse error: %s\n", err.c_str());
        state.reachable = false;
        return false;
    }

    state.reachable = true;
    state.on = doc["on"] | false;
    state.brightness = doc["bri"] | 0;
    state.preset = doc["ps"] | -1;

    JsonArray seg = doc["seg"].as<JsonArray>();
    if (seg && seg.size() > 0) {
        JsonArray col = seg[0]["col"][0].as<JsonArray>();
        if (col && col.size() >= 3) {
            state.r = col[0] | 0;
            state.g = col[1] | 0;
            state.b = col[2] | 0;
        }
        state.effect = seg[0]["fx"] | 0;
        state.palette = seg[0]["pal"] | 0;
    }

    return true;
}

bool WLEDClient::parseEffectsResponse(const String& response, std::vector<String>& effects) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.printf("[WLED] Effects parse error: %s\n", err.c_str());
        return false;
    }

    effects.clear();
    JsonArray arr = doc.as<JsonArray>();
    for (JsonVariant v : arr) {
        effects.push_back(v.as<String>());
    }
    return true;
}

bool WLEDClient::parsePresetsResponse(const String& response, std::vector<std::pair<int, String>>& presets, String* cacheJson) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.printf("[WLED] Presets parse error: %s\n", err.c_str());
        return false;
    }

    presets.clear();
    JsonDocument cacheDoc;
    JsonArray cacheArr = cacheDoc.to<JsonArray>();

    for (JsonPair kv : doc.as<JsonObject>()) {
        int id = atoi(kv.key().c_str());
        String fallback = "Preset " + String(id);
        const char* n = kv.value()["n"] | fallback.c_str();
        presets.push_back({id, String(n)});

        JsonObject obj = cacheArr.add<JsonObject>();
        obj["id"] = id;
        obj["n"] = n;
    }

    if (cacheJson) {
        serializeJson(cacheDoc, *cacheJson);
    }
    return true;
}

bool WLEDClient::httpGetInto(const String& path, uint32_t timeoutMs, String& response) {
    if (WiFi.status() != WL_CONNECTED || _host.isEmpty()) return false;
    HTTPClient http;
    String url = "http://" + _host + ":" + String(_port) + path;
    http.begin(url);
    http.setTimeout(timeoutMs);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[WLED] GET %s failed: %d\n", url.c_str(), code);
        http.end();
        return false;
    }
    response = http.getString();
    http.end();
    return true;
}

bool WLEDClient::httpPost(const String& path, const String& body) {
    if (WiFi.status() != WL_CONNECTED || _host.isEmpty()) return false;
    HTTPClient http;
    String url = "http://" + _host + ":" + String(_port) + path;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    int code = http.POST(body);
    http.end();
    if (code <= 0 || code >= 400) {
        Serial.printf("[WLED] POST %s failed: %d\n", path.c_str(), code);
        return false;
    }
    return true;
}

void WLEDClient::markReachable(bool reachable) {
    xSemaphoreTake(_dataMutex, portMAX_DELAY);
    _state.reachable = reachable;
    xSemaphoreGive(_dataMutex);
}

void WLEDClient::processRequest(const Request& req) {
    bool success = false;

    xSemaphoreTake(_queueMutex, portMAX_DELAY);
    switch (req.kind) {
        case RequestKind::FETCH_STATE: _queuedStateRefresh = false; break;
        case RequestKind::FETCH_EFFECTS: _queuedEffectsRefresh = false; break;
        case RequestKind::FETCH_PRESETS: _queuedPresetsRefresh = false; break;
        case RequestKind::FETCH_ALL: _queuedFetchAll = false; break;
        default: break;
    }
    xSemaphoreGive(_queueMutex);

    if (_host.isEmpty() || WiFi.status() != WL_CONNECTED) {
        _fetchStatus.store(FetchStatus::FAILED);
        markReachable(false);
        return;
    }

    if (req.kind == RequestKind::SEND_STATE) {
        success = httpPost("/json/state", String(req.body));
        if (success) {
            markReachable(true);
        }
    } else if (req.kind == RequestKind::FETCH_STATE) {
        String response;
        if (httpGetInto("/json/state", 3000, response)) {
            WLEDState nextState;
            if (parseStateResponse(response, nextState)) {
                xSemaphoreTake(_dataMutex, portMAX_DELAY);
                _state = nextState;
                xSemaphoreGive(_dataMutex);
                success = true;
            }
        }
    } else if (req.kind == RequestKind::FETCH_EFFECTS) {
        String response;
        if (httpGetInto("/json/effects", 3000, response)) {
            std::vector<String> effects;
            if (parseEffectsResponse(response, effects)) {
                xSemaphoreTake(_dataMutex, portMAX_DELAY);
                _effects = std::move(effects);
                configStore.setCachedEffects(response);
                xSemaphoreGive(_dataMutex);
                success = true;
            }
        }
    } else if (req.kind == RequestKind::FETCH_PRESETS) {
        String response;
        if (httpGetInto("/presets.json", 10000, response)) {
            std::vector<std::pair<int, String>> presets;
            String cacheStr;
            if (parsePresetsResponse(response, presets, &cacheStr)) {
                xSemaphoreTake(_dataMutex, portMAX_DELAY);
                _presets = std::move(presets);
                configStore.setCachedPresets(cacheStr);
                xSemaphoreGive(_dataMutex);
                success = true;
            }
        }
    } else if (req.kind == RequestKind::FETCH_ALL) {
        bool any = false;
        String response;
        if (httpGetInto("/json/state", 3000, response)) {
            WLEDState nextState;
            if (parseStateResponse(response, nextState)) {
                xSemaphoreTake(_dataMutex, portMAX_DELAY);
                _state = nextState;
                xSemaphoreGive(_dataMutex);
                any = true;
            }
        }
        if (httpGetInto("/json/effects", 3000, response)) {
            std::vector<String> effects;
            if (parseEffectsResponse(response, effects)) {
                xSemaphoreTake(_dataMutex, portMAX_DELAY);
                _effects = std::move(effects);
                configStore.setCachedEffects(response);
                xSemaphoreGive(_dataMutex);
                any = true;
            }
        }
        if (httpGetInto("/presets.json", 10000, response)) {
            std::vector<std::pair<int, String>> presets;
            String cacheStr;
            if (parsePresetsResponse(response, presets, &cacheStr)) {
                xSemaphoreTake(_dataMutex, portMAX_DELAY);
                _presets = std::move(presets);
                configStore.setCachedPresets(cacheStr);
                xSemaphoreGive(_dataMutex);
                any = true;
            }
        }
        success = any;
    }

    if (!success) markReachable(false);
    _fetchStatus.store(success ? FetchStatus::DONE : FetchStatus::FAILED);
}

bool WLEDClient::fetchState() {
    return requestStateRefresh();
}

bool WLEDClient::asyncFetchAll() {
    return enqueueRequest(RequestKind::FETCH_ALL, nullptr, true, false);
}

bool WLEDClient::requestStateRefresh() {
    uint32_t now = millis();
    if (_lastCommandMs != 0 && (now - _lastCommandMs) < POST_REFRESH_GUARD_MS) return false;
    if ((now - _lastStateRequestMs) < STATE_REFRESH_MIN_MS) return false;
    _lastStateRequestMs = now;
    return enqueueRequest(RequestKind::FETCH_STATE, nullptr, true, false);
}

bool WLEDClient::fetchEffects() {
    return enqueueRequest(RequestKind::FETCH_EFFECTS, nullptr, true, false);
}

bool WLEDClient::fetchPresets() {
    return enqueueRequest(RequestKind::FETCH_PRESETS, nullptr, true, false);
}

bool WLEDClient::sendState(const String& json) {
    return enqueueRequest(RequestKind::SEND_STATE, &json, false, true);
}

bool WLEDClient::togglePower() {
    return setPower(!_state.on);
}

bool WLEDClient::setPower(bool on) {
    String body;
    JsonDocument doc;
    doc["on"] = on;
    serializeJson(doc, body);
    return sendState(body);
}

bool WLEDClient::setBrightness(uint8_t bri) {
    String body;
    JsonDocument doc;
    doc["bri"] = bri;
    serializeJson(doc, body);
    return sendState(body);
}

bool WLEDClient::setColor(uint8_t r, uint8_t g, uint8_t b) {
    return setState(_state.on, _state.brightness, r, g, b);
}

bool WLEDClient::setState(bool on, uint8_t bri, uint8_t r, uint8_t g, uint8_t b) {
    String body;
    JsonDocument doc;
    doc["on"] = on;
    doc["bri"] = bri;
    JsonArray seg = doc["seg"].to<JsonArray>();
    JsonObject s0 = seg.add<JsonObject>();
    JsonArray col = s0["col"].to<JsonArray>();
    JsonArray c0 = col.add<JsonArray>();
    c0.add(r);
    c0.add(g);
    c0.add(b);
    serializeJson(doc, body);
    return sendState(body);
}

bool WLEDClient::setPreset(int presetId) {
    String body;
    JsonDocument doc;
    doc["ps"] = presetId;
    serializeJson(doc, body);
    return sendState(body);
}

bool WLEDClient::setEffect(int effectId) {
    String body;
    JsonDocument doc;
    JsonArray seg = doc["seg"].to<JsonArray>();
    JsonObject s0 = seg.add<JsonObject>();
    s0["fx"] = effectId;
    serializeJson(doc, body);
    return sendState(body);
}
