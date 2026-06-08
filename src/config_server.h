#pragma once

#include <Arduino.h>
#include <WebServer.h>

class ConfigServer {
public:
    void begin();
    void stop();
    void handleClient();
    bool isRunning() const { return _running; }

private:
    void handleRoot();
    void handleSave();
    void handleScan();
    void handleStatus();
    void handleReset();

    WebServer _server{8080};
    bool _running = false;
};

extern ConfigServer configServer;
