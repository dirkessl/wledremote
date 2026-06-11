#include "wled_client.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include "config_store.h"

namespace {
enum AsyncFetchFlags : uint8_t {
    FETCH_NONE    = 0,
    FETCH_STATE   = 1 << 0,
    FETCH_EFFECTS = 1 << 1,
    FETCH_PRESETS = 1 << 2,
    FETCH_POST    = 1 << 3,
    FETCH_ALL     = FETCH_STATE | FETCH_EFFECTS | FETCH_PRESETS,
};

bool httpGetInto(HTTPClient& http, const String& url, uint32_t timeoutMs, String& response) {
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
}

WLEDClient wledClient;

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

bool WLEDClient::fetchState() {
    return requestStateRefresh();
}

void asyncFetchTask(void* arg) {
    WLEDClient* client = static_cast<WLEDClient*>(arg);
    HTTPClient http;
    uint8_t flags = client->_fetchFlags;
    bool anySuccess = false;

    if (client->getHost().isEmpty() || WiFi.status() != WL_CONNECTED) {
        client->_fetchStatus.store(FetchStatus::FAILED);
        vTaskDelete(nullptr);
        return;
    }

    if (flags & FETCH_POST) {
        String body;
        xSemaphoreTake(client->_dataMutex, portMAX_DELAY);
        body = client->_pendingPostBody;
        client->_pendingPostBody = "";
        xSemaphoreGive(client->_dataMutex);

        if (!body.isEmpty()) {
            String url = "http://" + client->getHost() + ":" + String(client->_port) + "/json/state";
            http.begin(url);
            http.addHeader("Content-Type", "application/json");
            http.setTimeout(5000);
            int code = http.POST(body);
            http.end();

            if (code > 0 && code < 400) {
                anySuccess = true;
                flags |= FETCH_STATE;
            } else {
                Serial.printf("[WLED] POST /json/state failed: %d\n", code);
            }
        }
    }

    if (flags & FETCH_STATE) {
        String response;
        String url = "http://" + client->getHost() + ":" + String(client->_port) + "/json/state";
        if (httpGetInto(http, url, 3000, response)) {
            WLEDState nextState;
            if (client->parseStateResponse(response, nextState)) {
                xSemaphoreTake(client->_dataMutex, portMAX_DELAY);
                client->_state = nextState;
                xSemaphoreGive(client->_dataMutex);
                anySuccess = true;
            }
        }
    }

    if (flags & FETCH_EFFECTS) {
        String response;
        String url = "http://" + client->getHost() + ":" + String(client->_port) + "/json/effects";
        if (httpGetInto(http, url, 3000, response)) {
            std::vector<String> effects;
            if (client->parseEffectsResponse(response, effects)) {
                xSemaphoreTake(client->_dataMutex, portMAX_DELAY);
                client->_effects = std::move(effects);
                configStore.setCachedEffects(response);
                xSemaphoreGive(client->_dataMutex);
                anySuccess = true;
            }
        }
    }

    if (flags & FETCH_PRESETS) {
        String response;
        String url = "http://" + client->getHost() + ":" + String(client->_port) + "/presets.json";
        if (httpGetInto(http, url, 10000, response)) {
            std::vector<std::pair<int, String>> presets;
            String cacheStr;
            if (client->parsePresetsResponse(response, presets, &cacheStr)) {
                xSemaphoreTake(client->_dataMutex, portMAX_DELAY);
                client->_presets = std::move(presets);
                configStore.setCachedPresets(cacheStr);
                xSemaphoreGive(client->_dataMutex);
                anySuccess = true;
            }
        }
    }

    Serial.println("[WLED] Async worker complete");
    client->_fetchStatus.store(anySuccess ? FetchStatus::DONE : FetchStatus::FAILED);
    vTaskDelete(nullptr);
}

bool WLEDClient::startAsyncFetch(uint8_t flags, const String* postBody) {
    FetchStatus current = _fetchStatus.load();
    if (current == FetchStatus::IN_PROGRESS) return false;

    if (!_dataMutex) {
        _dataMutex = xSemaphoreCreateRecursiveMutex();
        if (!_dataMutex) {
            Serial.println("[WLED] Failed to create mutex");
            return false;
        }
    }

    if (postBody) {
        xSemaphoreTake(_dataMutex, portMAX_DELAY);
        _pendingPostBody = *postBody;
        xSemaphoreGive(_dataMutex);
        flags |= FETCH_POST;
    }

    _fetchFlags = flags;
    _fetchStatus.store(FetchStatus::IN_PROGRESS);

    BaseType_t ok = xTaskCreate(
        asyncFetchTask,
        "wled_fetch",
        12288,
        this,
        1,
        nullptr
    );

    if (ok != pdPASS) {
        Serial.println("[WLED] Failed to start async fetch task");
        _fetchStatus.store(FetchStatus::FAILED);
        return false;
    }

    return true;
}

bool WLEDClient::asyncFetchAll() {
    return startAsyncFetch(FETCH_ALL);
}

bool WLEDClient::requestStateRefresh() {
    return startAsyncFetch(FETCH_STATE);
}

bool WLEDClient::fetchEffects() {
    return startAsyncFetch(FETCH_EFFECTS);
}

bool WLEDClient::fetchPresets() {
    return startAsyncFetch(FETCH_PRESETS);
}

String WLEDClient::httpGet(const String& path) {
    if (WiFi.status() != WL_CONNECTED || _host.isEmpty()) return "";

    HTTPClient http;
    String url = "http://" + _host + ":" + String(_port) + path;
    http.begin(url);
    http.setTimeout(3000);
    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        Serial.printf("[WLED] GET %s failed: %d\n", path.c_str(), code);
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();
    return payload;
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

bool WLEDClient::sendState(const String& json) {
    return startAsyncFetch(FETCH_NONE, &json);
}

bool WLEDClient::togglePower() {
    return setPower(!_state.on);
}

bool WLEDClient::setPower(bool on) {
    String body;
    JsonDocument doc;
    doc["on"] = on;
    serializeJson(doc, body);
    bool ok = sendState(body);
    if (ok) _state.on = on;
    return ok;
}

bool WLEDClient::setBrightness(uint8_t bri) {
    String body;
    JsonDocument doc;
    doc["bri"] = bri;
    serializeJson(doc, body);
    bool ok = sendState(body);
    if (ok) _state.brightness = bri;
    return ok;
}

bool WLEDClient::setColor(uint8_t r, uint8_t g, uint8_t b) {
    String body;
    JsonDocument doc;
    JsonArray seg = doc["seg"].to<JsonArray>();
    JsonObject s0 = seg.add<JsonObject>();
    JsonArray col = s0["col"].to<JsonArray>();
    JsonArray c0 = col.add<JsonArray>();
    c0.add(r);
    c0.add(g);
    c0.add(b);
    serializeJson(doc, body);
    bool ok = sendState(body);
    if (ok) {
        _state.r = r;
        _state.g = g;
        _state.b = b;
    }
    return ok;
}

bool WLEDClient::setPreset(int presetId) {
    String body;
    JsonDocument doc;
    doc["ps"] = presetId;
    serializeJson(doc, body);
    bool ok = sendState(body);
    if (ok) _state.preset = presetId;
    return ok;
}

bool WLEDClient::setEffect(int effectId) {
    String body;
    JsonDocument doc;
    JsonArray seg = doc["seg"].to<JsonArray>();
    JsonObject s0 = seg.add<JsonObject>();
    s0["fx"] = effectId;
    serializeJson(doc, body);
    bool ok = sendState(body);
    if (ok) _state.effect = effectId;
    return ok;
}
