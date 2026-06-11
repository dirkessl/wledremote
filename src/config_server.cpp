#include "config_server.h"
#include "homekit_bridge.h"
#include "config_store.h"
#include "wled_discovery.h"
#include "wled_client.h"
#include <ArduinoJson.h>
#include <WiFi.h>

ConfigServer configServer;

static const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WLED Remote Setup</title>
<style>
body{font-family:sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:20px}
h1{color:#00bfff;margin-top:0}
.card{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
input,select{width:100%;padding:10px;margin:6px 0;border:1px solid #444;border-radius:4px;background:#0f3460;color:#eee;box-sizing:border-box}
button{background:#00bfff;color:#000;border:none;padding:12px 24px;border-radius:4px;cursor:pointer;font-size:16px;margin:4px}
button:hover{background:#0099cc}
button.danger{background:#ff4444}
.device{padding:8px;margin:4px 0;background:#0f3460;border-radius:4px;cursor:pointer;border:1px solid transparent}
.device:hover{border-color:#00bfff}
.status{padding:4px 8px;border-radius:3px;font-size:12px}
.online{background:#00ff88;color:#000}
.offline{background:#ff4444;color:#fff}
</style></head><body>
)rawliteral";

static const char HTML_FOOTER[] PROGMEM = R"rawliteral(
</body></html>
)rawliteral";

void ConfigServer::begin() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/save", HTTP_POST, [this]() { handleSave(); });
    _server.on("/scan", HTTP_GET, [this]() { handleScan(); });
    _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/reset", HTTP_POST, [this]() { handleReset(); });
    _server.on("/save_hostname", HTTP_POST, [this]() { handleSaveHostname(); });
    _server.on("/reset_homekit", HTTP_POST, [this]() { handleResetHomeKit(); });
    _server.begin();
    _running = true;
    Serial.println("[Config] Web server started on port 8080");
}

void ConfigServer::stop() {
    _server.stop();
    _running = false;
}

void ConfigServer::handleClient() {
    if (_running) {
        _server.handleClient();
    }
}

void ConfigServer::handleRoot() {
    String html;
    html.reserve(4096);
    html += FPSTR(HTML_HEADER);
    html += F("<h1>WLED Remote Setup</h1>");

    // Current configuration
    html += F("<div class='card'><h2>WLED Connection</h2>");
    if (configStore.hasWLEDConfig()) {
        html += F("<p>Current: <strong>");
        html += configStore.getWLEDHost();
        html += F(":");
        html += String(configStore.getWLEDPort());
        html += F("</strong> <span class='status ");
        html += wledClient.isReachable() ? "online'>Connected" : "offline'>Disconnected";
        html += F("</span></p>");
    }

    html += F("<form action='/save' method='post'>");
    html += F("<label>WLED IP Address:</label>");
    html += F("<input type='text' name='host' placeholder='192.168.1.x' value='");
    html += configStore.getWLEDHost();
    html += F("'>");
    html += F("<label>Port:</label>");
    html += F("<input type='number' name='port' value='");
    html += String(configStore.getWLEDPort());
    html += F("'>");
    html += F("<button type='submit'>Save</button>");
    html += F("</form></div>");

    // Hostname configuration
    html += F("<div class='card'><h2>Hostname</h2>");
    html += F("<form action='/save_hostname' method='post'>");
    html += F("<input type='text' name='hostname' value='");
    html += configStore.getHostname();
    html += F("'>");
    html += F("<button type='submit'>Save</button>");
    html += F("</form></div>");

    // Discovered devices
    html += F("<div class='card'><h2>Discovered Devices</h2>");
    html += F("<button onclick=\"scanDevices()\">Scan Network</button>");
    html += F("<div id='devices'></div></div>");

    // Settings
    html += F("<div class='card'><h2>Settings</h2>");
    html += F("<form action='/reset_homekit' method='post' style='display:inline;'>");
    html += F("<button class='danger' type='submit' onclick='return confirm(\"Reset HomeKit pairing?\")'>Reset HomeKit</button>");
    html += F("</form>");
    html += F("<form action='/reset' method='post' style='display:inline;'>");
    html += F("<button class='danger' type='submit' onclick='return confirm(\"Reset all settings?\")'>Factory Reset</button>");
    html += F("</form></div>");

    html += F(R"rawliteral(
<script>
function selectDevice(ip, port) {
    document.querySelector('[name=host]').value = ip;
    document.querySelector('[name=port]').value = port;
}

function scanDevices() {
    const devicesEl = document.getElementById('devices');
    devicesEl.innerHTML = 'Scanning...';

    fetch('/scan')
        .then(r => r.json())
        .then(d => {
            let h = '';
            d.forEach(x => {
                h += `<div class="device" onclick="selectDevice('${x.ip}', ${x.port})">${x.name} (${x.ip})</div>`;
            });
            devicesEl.innerHTML = h || 'No devices found';
        })
        .catch(() => {
            devicesEl.innerHTML = 'Scan failed';
        });
}
</script>
)rawliteral");

    html += FPSTR(HTML_FOOTER);

    _server.send(200, "text/html", html);
}

void ConfigServer::handleSave() {
    String host = _server.arg("host");
    String portStr = _server.arg("port");

    if (host.length() == 0) {
        _server.send(400, "text/plain", "Host is required");
        return;
    }

    uint16_t port = portStr.length() > 0 ? portStr.toInt() : 80;

    configStore.setWLEDHost(host);
    configStore.setWLEDPort(port);
    wledClient.setHost(host, port);

    String html;
    html += FPSTR(HTML_HEADER);
    html += F("<h1>Saved!</h1><p>WLED set to: ");
    html += host + ":" + String(port);
    html += F("</p><p><a href='/'>Back</a></p>");
    html += FPSTR(HTML_FOOTER);
    _server.send(200, "text/html", html);
}

void ConfigServer::handleScan() {
    wledDiscovery.scan();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& device : wledDiscovery.getDevices()) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = device.name;
        obj["ip"] = device.ip;
        obj["port"] = device.port;
    }

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void ConfigServer::handleStatus() {
    JsonDocument doc;
    doc["wled_configured"] = configStore.hasWLEDConfig();
    doc["wled_host"] = configStore.getWLEDHost();
    doc["wled_port"] = configStore.getWLEDPort();
    doc["wled_reachable"] = wledClient.isReachable();
    doc["wifi_ip"] = WiFi.localIP().toString();

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void ConfigServer::handleReset() {
    configStore.factoryReset();
    String html;
    html += FPSTR(HTML_HEADER);
    html += F("<h1>Reset Complete</h1><p>Device will restart...</p>");
    html += FPSTR(HTML_FOOTER);
    _server.send(200, "text/html", html);
    delay(1000);
    ESP.restart();
}

void ConfigServer::handleSaveHostname() {
    String hostname = _server.arg("hostname");
    if (hostname.length() > 0) {
        configStore.setHostname(hostname);
    }
    String html;
    html += FPSTR(HTML_HEADER);
    html += F("<h1>Saved!</h1><p>Hostname set to: ");
    html += hostname;
    html += F("</p><p>Requires restart to take effect.</p><p><a href='/'>Back</a></p>");
    html += FPSTR(HTML_FOOTER);
    _server.send(200, "text/html", html);
}

void ConfigServer::handleResetHomeKit() {
    homeKitBridge.reset();
    String html;
    html += FPSTR(HTML_HEADER);
    html += F("<h1>HomeKit Reset Complete</h1><p>Device will restart...</p>");
    html += FPSTR(HTML_FOOTER);
    _server.send(200, "text/html", html);
    delay(1000);
    ESP.restart();
}
