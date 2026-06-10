#include "wled_client.h"
#include <HTTPClient.h>
#include <WiFi.h>

WLEDClient wledClient;

void WLEDClient::setHost(const String& host, uint16_t port) {
    _host = host;
    _port = port;
}

bool WLEDClient::fetchState() {
    String response = httpGet("/json/state");
    if (response.isEmpty()) {
        _state.reachable = false;
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.printf("[WLED] JSON parse error: %s\n", err.c_str());
        _state.reachable = false;
        return false;
    }

    _state.reachable = true;
    _state.on = doc["on"] | false;
    _state.brightness = doc["bri"] | 0;
    _state.preset = doc["ps"] | -1;

    JsonArray seg = doc["seg"].as<JsonArray>();
    if (seg && seg.size() > 0) {
        JsonArray col = seg[0]["col"][0].as<JsonArray>();
        if (col && col.size() >= 3) {
            _state.r = col[0] | 0;
            _state.g = col[1] | 0;
            _state.b = col[2] | 0;
        }
        _state.effect = seg[0]["fx"] | 0;
        _state.palette = seg[0]["pal"] | 0;
    }

    return true;
   }

// ---- Async fetch via FreeRTOS task ----

static void asyncFetchTask(void* arg) {
    WLEDClient* client = static_cast<WLEDClient*>(arg);
    HTTPClient http;

    // Fetch effects first (small payload, ~2-3KB)
        if (!client->getHost().isEmpty()) {
            String url = "http://" + client->getHost() + ":" + String(client->_port) + "/json/effects";

        http.begin(url);
        http.setTimeout(3000);
        int code = http.GET();

        if (code == HTTP_CODE_OK) {
            String response = http.getString();
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, response);
            if (!err) {
                xSemaphoreTake(client->_dataMutex, portMAX_DELAY);
                client->_effects.clear();
                JsonArray arr = doc.as<JsonArray>();
                for (JsonVariant v : arr) {
                    client->_effects.push_back(v.as<String>());
                    }
                xSemaphoreGive(client->_dataMutex);
                }
            }
        http.end();
        }

    // Fetch presets (larger payload, can be ~50KB)
        if (!client->getHost().isEmpty()) {
            url = "http://" + client->getHost() + ":" + String(client->_port) + "/presets.json";

        http.begin(url);
        http.setTimeout(10000);  // longer timeout for larger payload
        code = http.GET();

        if (code == HTTP_CODE_OK) {
            String response = http.getString();
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, response);
            if (!err) {
                xSemaphoreTake(client->_dataMutex, portMAX_DELAY);
                client->_presets.clear();
                for (JsonPair kv : doc.as<JsonObject>()) {
                    int id = atoi(kv.key().c_str());
                    String fallback = "Preset " + String(id);
                    const char* n = kv.value()["n"] | fallback.c_str();
                    client->_presets.push_back({id, String(n)});
                    }
                xSemaphoreGive(client->_dataMutex);
                }
            }
        http.end();
        }

    Serial.println("[WLED] Async fetch complete");
     client->_fetchStatus.store(FetchStatus::DONE);
    vTaskDelete(NULL);
     }

bool WLEDClient::asyncFetchAll() {
    FetchStatus current = _fetchStatus.load();
    if (current == FetchStatus::IN_PROGRESS) return false;

     // Create mutex on first use
    if (!_dataMutex) {
        _dataMutex = xSemaphoreCreateRecursiveMutex();
        if (!_dataMutex) {
            Serial.println("[WLED] Failed to create mutex");
            return false;
            }
        }

    _fetchStatus.store(FetchStatus::IN_PROGRESS);
    Serial.println("[WLED] Starting async fetch...");

    BaseType_t ret = xTaskCreate(
         asyncFetchTask,
         "wledFetch", 4096, this,   // 4KB stack for HTTPClient + JSON
        1,                          // priority
        NULL);                      // handle not needed

     if (ret != pdPASS) {
         Serial.println("[WLED] Failed to create fetch task");
          _fetchStatus.store(FetchStatus::FAILED);
         return false;
           }

    return true;
      }

bool WLEDClient::fetchEffects() {
     String response = httpGet("/json/effects");
     if (response.isEmpty()) return false;

     JsonDocument doc;
     DeserializationError err = deserializeJson(doc, response);
     if (err) return false;

       _effects.clear();
     JsonArray arr = doc.as<JsonArray>();
     for (JsonVariant v : arr) {
          _effects.push_back(v.as<String>());
       }
     return true;
      }

bool WLEDClient::fetchPresets() {
     String response = httpGet("/presets.json");
     if (response.isEmpty()) return false;

     JsonDocument doc;
     DeserializationError err = deserializeJson(doc, response);
     if (err) return false;

       _presets.clear();
     for (JsonPair kv : doc.as<JsonObject>()) {
         int id = atoi(kv.key().c_str());
         String fallback = "Preset " + String(id);
         const char* n = kv.value()["n"] | fallback.c_str();
           _presets.push_back({id, String(n)});
       }
     return true;
      }

bool WLEDClient::togglePower() {
     return sendState("{\"on\":\"t\"}");
       }

bool WLEDClient::setPower(bool on) {
     return sendState(on ? "{\"on\":true}" : "{\"on\":false}");
       }

bool WLEDClient::setBrightness(uint8_t bri) {
    return sendState("{\"bri\":" + String(bri) + "}");
}

bool WLEDClient::setColor(uint8_t r, uint8_t g, uint8_t b) {
    String json = "{\"seg\":[{\"col\":[[" + String(r) + "," + String(g) + "," + String(b) + "]]}]}";
    return sendState(json);
}

bool WLEDClient::setPreset(int presetId) {
    return sendState("{\"ps\":" + String(presetId) + "}");
}

bool WLEDClient::setEffect(int effectId) {
    return sendState("{\"seg\":[{\"fx\":" + String(effectId) + "}]}");
}

bool WLEDClient::sendState(const String& json) {
    return httpPost("/json/state", json);
}

String WLEDClient::httpGet(const String& path) {
    if (_host.isEmpty()) return "";

    HTTPClient http;
    String url = "http://" + _host + ":" + String(_port) + path;
    http.begin(url);
    http.setTimeout(3000);

    int code = http.GET();
    String result;
    if (code == HTTP_CODE_OK) {
        result = http.getString();
    } else {
        Serial.printf("[WLED] GET %s failed: %d\n", path.c_str(), code);
    }
    http.end();
    return result;
}

bool WLEDClient::httpPost(const String& path, const String& body) {
    if (_host.isEmpty()) return false;

    HTTPClient http;
    String url = "http://" + _host + ":" + String(_port) + path;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(3000);

    int code = http.POST(body);
    http.end();

    if (code == HTTP_CODE_OK) {
        return true;
    }
    Serial.printf("[WLED] POST %s failed: %d\n", path.c_str(), code);
    return false;
}
