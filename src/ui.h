#pragma once

#include <Arduino.h>
#include "lgfx_setup.h"
#include "wled_client.h"
#include "wled_discovery.h"
#include "button_handler.h"

// Application states
enum class AppScreen {
    BOOT,
    AP_MODE,
    WIFI_CONNECTING,
    WLED_CONNECTING,
    SHOW_IP,
    WLED_SCAN,
    WLED_SELECT,
    MAIN_STATUS,
    MENU,
    COLOR_PICKER,
    BRIGHTNESS,
    PRESETS,
    EFFECTS,
    RECOVERING,
    LOADING
};

// Menu items for the main menu
enum class MenuItem {
    COLORS = 0,
    PRESETS,
    EFFECTS,
    HOMEKIT,
    SETTINGS,
    MENU_COUNT
};

class UI {
public:
    void begin(LGFX* display);
    void update();

    // Screen transitions
    void showBoot();
    void showAPMode(const String& apName);
    void showWifiConnecting();
    void showWledConnecting();
    void showIP(const String& ip);
    void showWLEDScan();
    void showWLEDSelect(const std::vector<WLEDDevice>& devices);
    void showMainStatus(const WLEDState& state, bool isWifiConnected = true);
    void showMenu();
    void showColorPicker();
    void showBrightness(uint8_t currentBri);
    void showPresets(const std::vector<std::pair<int, String>>& presets);
    void showEffects(const std::vector<String>& effects);
    void showError(const String& message);
    void showMessage(const String& title, const String& message);
    void showHomeKitPairing(const String& setupCode, const String& setupURI);
    void showRecovering(const String& msg);
    void showLoading();

      // Handle button input for current screen
    // Returns true if the UI handled the event (no further processing needed)
    bool handleLeft(ButtonEvent event);
    bool handleRight(ButtonEvent event);
    bool handleRotation(int8_t direction);

    // Getters
    AppScreen getCurrentScreen() const { return _screen; }
    int getSelectedIndex() const { return _selectedIndex; }

    // Activity tracking for display dimming
    void resetActivityTimer();
    bool shouldDim() const;

private:
    void drawHeader(const String& title);
    void drawFooter(const String& leftLabel, const String& rightLabel);
    void drawMenuItem(int y, const String& text, bool selected);
    void drawProgressBar(int x, int y, int w, int h, uint8_t value, uint32_t color);
    void drawColorSwatch(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);

    LGFX* _tft = nullptr;
    LGFX_Sprite _sprite;
    AppScreen _screen = AppScreen::BOOT;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    int _itemCount = 0;
    uint32_t _lastActivity = 0;

    static constexpr uint32_t DIM_TIMEOUT_MS = 30000;
    static constexpr int SCREEN_W = 170;
    static constexpr int SCREEN_H = 320;
    static constexpr int HEADER_H = 30;
    static constexpr int FOOTER_H = 24;
    static constexpr int ITEM_H = 28;
    static constexpr int CONTENT_Y = HEADER_H + 4;
    static constexpr int CONTENT_H = SCREEN_H - HEADER_H - FOOTER_H - 8;
};

extern UI ui;
