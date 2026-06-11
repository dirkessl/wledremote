#include "ui.h"
#include <math.h>

UI ui;

// MPS Logo 80x80 bitmap (1=draw pixel)
static const uint8_t mpsLogo[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1F, 0xE0, 0x07, 0xF8, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x07, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x07, 0x80, 0x0C, 0x1C, 0x00, 0xE0, 0x00, 0x00,
    0x00, 0x00, 0x1E, 0x07, 0x8C, 0x1E, 0x00, 0x38, 0x00, 0x00,
    0x00, 0x00, 0x38, 0x0F, 0x0C, 0x1B, 0x18, 0x1E, 0x00, 0x00,
    0x00, 0x00, 0xE0, 0x06, 0x0C, 0x3E, 0x3A, 0x07, 0x00, 0x00,
    0x00, 0x01, 0xC1, 0xC7, 0x8C, 0x3C, 0x36, 0x03, 0x80, 0x00,
    0x00, 0x03, 0x87, 0x87, 0x0F, 0x30, 0x36, 0x01, 0xC0, 0x00,
    0x00, 0x07, 0x03, 0xC3, 0xCF, 0x30, 0x6C, 0x10, 0x60, 0x00,
    0x00, 0x0C, 0x00, 0xC3, 0xC0, 0x00, 0x6C, 0x38, 0x30, 0x00,
    0x00, 0x18, 0x70, 0x60, 0x00, 0x00, 0x78, 0x74, 0x18, 0x00,
    0x00, 0x38, 0xE0, 0x60, 0x00, 0x00, 0x10, 0xFE, 0x0C, 0x00,
    0x00, 0x70, 0xF0, 0x20, 0x7F, 0xFE, 0x01, 0xFC, 0x0E, 0x00,
    0x00, 0x60, 0x38, 0x01, 0xFF, 0xFF, 0xC1, 0xB8, 0x07, 0x00,
    0x00, 0xC0, 0x1C, 0x0F, 0xFF, 0xFF, 0xF0, 0x70, 0x43, 0x00,
    0x01, 0x86, 0x0C, 0x1F, 0xFF, 0x7F, 0xF8, 0x61, 0xC1, 0x80,
    0x01, 0x87, 0x00, 0x7F, 0xFF, 0x7F, 0xFE, 0x03, 0xC1, 0xC0,
    0x03, 0x03, 0xC0, 0xFF, 0xFF, 0x7F, 0xFF, 0x0F, 0xF0, 0xC0,
    0x03, 0x00, 0xE1, 0xFF, 0x7F, 0x7E, 0xFF, 0x8F, 0x80, 0x60,
    0x06, 0x20, 0x63, 0xFF, 0x3F, 0x7C, 0xFF, 0xC3, 0x00, 0x60,
    0x06, 0x3C, 0x07, 0xFF, 0xBF, 0x7D, 0xFF, 0xE3, 0x00, 0x30,
    0x0C, 0x3F, 0x0F, 0xFF, 0x9F, 0x79, 0xFF, 0xF2, 0x04, 0x30,
    0x0C, 0x1F, 0x8F, 0xE7, 0xDF, 0x7B, 0xE7, 0xF8, 0x06, 0x30,
    0x18, 0x7E, 0x1F, 0xF3, 0xFF, 0xFB, 0xCF, 0xF8, 0x1E, 0x18,
    0x18, 0xFF, 0x3F, 0x79, 0xFF, 0xFF, 0x9E, 0xFC, 0xFE, 0x18,
    0x18, 0xF8, 0x3F, 0xFE, 0xF0, 0x0F, 0x3F, 0xFC, 0xE0, 0x18,
    0x10, 0x3E, 0x7F, 0xFF, 0xC0, 0x03, 0xFF, 0xFE, 0x00, 0x0C,
    0x30, 0x06, 0x7F, 0xFF, 0x80, 0x01, 0xFF, 0xFE, 0x00, 0x0C,
    0x30, 0x00, 0x78, 0xF7, 0xC0, 0x03, 0xEF, 0x9E, 0x00, 0x0C,
    0x30, 0x00, 0xFE, 0x73, 0xF0, 0x07, 0xCF, 0x7F, 0x00, 0x0C,
    0x30, 0x00, 0xFF, 0xF1, 0xF8, 0x1F, 0x8F, 0xFF, 0x00, 0x04,
    0x20, 0x00, 0xFF, 0xF0, 0xFC, 0x3F, 0x0F, 0xFF, 0x00, 0x06,
    0x20, 0x00, 0xFF, 0xF0, 0x3E, 0x7C, 0x0F, 0xFF, 0x00, 0x06,
    0x20, 0xE0, 0xF0, 0xF0, 0x1F, 0xF8, 0x0F, 0x0F, 0x07, 0x06,
    0x21, 0xF8, 0xFF, 0xF0, 0x0F, 0xF0, 0x0F, 0xFF, 0x8F, 0x86,
    0x63, 0xB8, 0xFF, 0xF0, 0x07, 0xE0, 0x0F, 0xFF, 0x9D, 0xC6,
    0x63, 0x18, 0xFF, 0xF0, 0x03, 0xC0, 0x0F, 0xFF, 0x98, 0xC6,
    0x63, 0x98, 0xE0, 0xF0, 0x00, 0x00, 0x0F, 0x07, 0x9D, 0xC6,
    0x21, 0xF8, 0xFF, 0xF0, 0x10, 0x08, 0x0F, 0xFF, 0x8F, 0x86,
    0x20, 0xE0, 0xFF, 0xF0, 0x18, 0x18, 0x0F, 0xFF, 0x07, 0x06,
    0x20, 0x00, 0xFF, 0xF1, 0x1C, 0x38, 0x8F, 0xFF, 0x00, 0x04,
    0x30, 0x00, 0xFE, 0xF1, 0x0E, 0x70, 0x8F, 0x3F, 0x00, 0x04,
    0x30, 0x00, 0xF9, 0xF1, 0x8F, 0xF1, 0x8F, 0x9F, 0x00, 0x0C,
    0x30, 0x00, 0x7F, 0xF1, 0x87, 0xE1, 0x8F, 0xFE, 0x00, 0x0C,
    0x30, 0x00, 0x7F, 0xF1, 0xC7, 0xE3, 0x8F, 0xFE, 0x00, 0x0C,
    0x10, 0x00, 0x7F, 0xF1, 0xC3, 0xC3, 0x8F, 0xFE, 0x00, 0x0C,
    0x18, 0x00, 0x3D, 0xF1, 0xE3, 0xC7, 0x8F, 0xBC, 0x00, 0x18,
    0x18, 0x00, 0x3F, 0xF1, 0xE1, 0x87, 0x8F, 0xFC, 0x00, 0x18,
    0x18, 0x00, 0x1F, 0xF1, 0xF1, 0x8F, 0x8F, 0xF8, 0x00, 0x18,
    0x0C, 0x00, 0x0F, 0xF1, 0xF1, 0x8F, 0x8F, 0xF0, 0x00, 0x30,
    0x0C, 0x01, 0x0F, 0xF1, 0xF8, 0x0F, 0x8F, 0xF0, 0x00, 0x30,
    0x0E, 0x07, 0x87, 0xF1, 0xF8, 0x1F, 0x8F, 0xE0, 0xC0, 0x30,
    0x06, 0x06, 0xC3, 0xF1, 0xF8, 0x1F, 0xCF, 0xC1, 0xC0, 0x60,
    0x07, 0x16, 0xC1, 0xF9, 0xFC, 0x3F, 0xDF, 0x83, 0x38, 0x60,
    0x03, 0x16, 0x00, 0xFF, 0xFC, 0x3F, 0xFF, 0x01, 0xFC, 0xC0,
    0x01, 0x9E, 0x00, 0x3F, 0xFE, 0x7F, 0xFE, 0x00, 0xCD, 0xC0,
    0x01, 0x8C, 0x00, 0x1F, 0xFE, 0x7F, 0xF8, 0x00, 0x39, 0x80,
    0x00, 0xC0, 0x06, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x33, 0x00,
    0x00, 0xE0, 0x07, 0x81, 0xFF, 0xFF, 0x80, 0xE0, 0x07, 0x00,
    0x00, 0x70, 0x07, 0x80, 0x3F, 0xFC, 0x01, 0xF0, 0x06, 0x00,
    0x00, 0x30, 0x0E, 0x00, 0x00, 0x00, 0x01, 0x98, 0x0C, 0x00,
    0x00, 0x18, 0x0C, 0x06, 0x00, 0x00, 0x00, 0xD8, 0x18, 0x00,
    0x00, 0x0C, 0x18, 0x06, 0x80, 0x00, 0xC0, 0xEC, 0x30, 0x00,
    0x00, 0x06, 0x10, 0x0C, 0xC3, 0xC0, 0x60, 0x78, 0x60, 0x00,
    0x00, 0x03, 0x80, 0x0D, 0x83, 0xE0, 0x60, 0x30, 0xC0, 0x00,
    0x00, 0x01, 0xC0, 0x0D, 0x83, 0x60, 0x60, 0x03, 0x80, 0x00,
    0x00, 0x00, 0xE0, 0x0D, 0x83, 0x30, 0x30, 0x07, 0x00, 0x00,
    0x00, 0x00, 0x78, 0x0F, 0x03, 0x20, 0x30, 0x1E, 0x00, 0x00,
    0x00, 0x00, 0x1C, 0x06, 0x03, 0xE0, 0x30, 0x38, 0x00, 0x00,
    0x00, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x00,
    0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1F, 0xC0, 0x03, 0xF8, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static constexpr int LOGO_W = 80;
static constexpr int LOGO_H = 80;

static void drawLogo(LGFX_Sprite& sprite, int16_t ox, int16_t oy, uint32_t color) {
    static constexpr int ROW_BYTES = LOGO_W / 8;
    for (int row = 0; row < LOGO_H; row++) {
        for (int bx = 0; bx < ROW_BYTES; bx++) {
            uint8_t b = pgm_read_byte(&mpsLogo[row * ROW_BYTES + bx]);
            for (int bit = 0; bit < 8; bit++) {
                if (b & (0x80 >> bit)) {
                    sprite.drawPixel(ox + bx * 8 + bit, oy + row, color);
                }
            }
        }
    }
}

// Color definitions
static constexpr uint32_t COL_BG       = 0x000000U;
static constexpr uint32_t COL_TEXT     = 0xFFFFFFU;
static constexpr uint32_t COL_HEADER   = 0x1A1A2EU;
static constexpr uint32_t COL_ACCENT   = 0x00BFFFU;
static constexpr uint32_t COL_SUCCESS  = 0x00FF88U;
static constexpr uint32_t COL_WARNING  = 0xFF4444U;
static constexpr uint32_t COL_DIM      = 0x888888U;
static constexpr uint32_t COL_SELECT   = 0x333355U;

void UI::begin(LGFX* display) {
    _tft = display;
    _sprite.setColorDepth(16);
    _sprite.createSprite(SCREEN_W, SCREEN_H);
    _lastActivity = millis();
}

void UI::update() {
    // Push sprite to display
    _sprite.pushSprite(_tft, 0, 0);
}

void UI::showBoot() {
    _screen = AppScreen::BOOT;
    _sprite.fillScreen(0x1C1C1EU);  // iOS dark bg

    // Logo centered (native 80x80)
    int logoX = (SCREEN_W - LOGO_W) / 2;
    int logoY = 70;
    drawLogo(_sprite, logoX, logoY, 0xFFFFFFU);

    // App name
    _sprite.setTextColor(0xFFFFFFU);
    _sprite.setTextSize(2);
    int nameW = 11 * 12;  // "WLED Remote" at size 2
    _sprite.setCursor((SCREEN_W - nameW) / 2, logoY + LOGO_H + 16);
    _sprite.print("WLED Remote");

    // Studio name
    _sprite.setTextColor(0x8E8E93U);
    _sprite.setTextSize(1);
    int studioW = 19 * 6;  // "Mittelpunkt Studios"
    _sprite.setCursor((SCREEN_W - studioW) / 2, logoY + LOGO_H + 42);
    _sprite.print("Mittelpunkt Studios");

    update();
}

void UI::showAPMode(const String& apName) {
    _screen = AppScreen::AP_MODE;
    _sprite.fillScreen(0x1C1C1EU);

    // WiFi icon (simple placeholder)
    int cx = SCREEN_W / 2;
    _sprite.drawCircle(cx, 55, 28, 0xFFAA00U);
    _sprite.fillCircle(cx, 55, 5, 0xFFAA00U);

    _sprite.setTextColor(0xFFFFFFU);
    _sprite.setTextSize(2);
    int titleW = 9 * 12;  // "WiFi Setup"
    _sprite.setCursor((SCREEN_W - titleW) / 2, 95);
    _sprite.print("WiFi Setup");

    // AP name
    _sprite.setTextColor(0xFFAA00U);
    _sprite.setTextSize(1);
    _sprite.setCursor((SCREEN_W - apName.length() * 6) / 2, 124);
    _sprite.print(apName);

    // Instruction
    _sprite.setTextColor(0x8E8E93U);
    _sprite.setTextSize(1);
    _sprite.setCursor(4, 148);
    _sprite.print("1. Connect to WiFi above");
    _sprite.setCursor(4, 162);
    _sprite.print("2. Open browser, go to:");

    // IP large
    _sprite.setTextColor(0x30D158U);
    _sprite.setTextSize(2);
    int ipW = 11 * 12; // "192.168.4.1"
    _sprite.setCursor((SCREEN_W - ipW) / 2, 182);
    _sprite.print("192.168.4.1");

    update();
}

void UI::showWifiConnecting() {
    _screen = AppScreen::WIFI_CONNECTING;
    _sprite.fillScreen(0x1C1C1EU);  // iOS dark bg

    int logoX = (SCREEN_W - LOGO_W) / 2;
    int logoY = 60;
    drawLogo(_sprite, logoX, logoY, 0xFFFFFFU);

    _sprite.setTextColor(0xFFFFFFU);
    _sprite.setTextSize(2);
    int textW = 13 * 12; // "Connecting..."
    _sprite.setCursor((SCREEN_W - textW) / 2, logoY + LOGO_H + 20);
    _sprite.print("Connecting...");

    _sprite.setTextColor(0x8E8E93U);
    _sprite.setTextSize(1);
    int subW = 14 * 6;
    _sprite.setCursor((SCREEN_W - subW) / 2, logoY + LOGO_H + 48);
    _sprite.print("Please wait..");

    update();
}

void UI::showWledConnecting() {
    _screen = AppScreen::WLED_CONNECTING;
    _sprite.fillScreen(0x1C1C1EU);  // iOS dark bg

    int logoX = (SCREEN_W - LOGO_W) / 2;
    int logoY = 60;
    drawLogo(_sprite, logoX, logoY, 0xFFFFFFU);

    _sprite.setTextColor(0xFFFFFFU);
    _sprite.setTextSize(2);
    int textW = 13 * 12; // "Connecting..."
    _sprite.setCursor((SCREEN_W - textW) / 2, logoY + LOGO_H + 20);
    _sprite.print("Connecting...");

    _sprite.setTextColor(0x8E8E93U);
    _sprite.setTextSize(1);
    int subW = 14 * 6;
    _sprite.setCursor((SCREEN_W - subW) / 2, logoY + LOGO_H + 48);
    _sprite.print("Please wait..");

    update();
}

void UI::showIP(const String& ip) {
    _screen = AppScreen::SHOW_IP;
    _sprite.fillScreen(0x1C1C1EU);

    String url = "http://" + ip + ":8080";

    _sprite.setTextColor(0xFFFFFFU);
    _sprite.setTextSize(2);
    _sprite.setCursor(52, 16);
    _sprite.print("Settings");

    _sprite.setTextColor(0x8E8E93U);
    _sprite.setTextSize(1);
    _sprite.setCursor(16, 42);
    _sprite.print("Scan to open config UI");

    // Optional white background behind QR
    int qrSize = 100;
    int qrX = (SCREEN_W - qrSize) / 2;
    int qrY = CONTENT_Y + 70;
    _sprite.fillRoundRect(qrX - 6, qrY - 6, qrSize + 12, qrSize + 12, 8, 0xFFFFFFU);

    // Footer in sprite too
    _sprite.setTextColor(COL_DIM);
    _sprite.setTextSize(1);
    _sprite.setCursor(10, SCREEN_H - FOOTER_H + 8);
    drawFooter(" ", "Back");

    update();  // push sprite first

    // Draw QR last, directly on display
    _tft->qrcode(url.c_str(), qrX, qrY, qrSize, 4);
}



void UI::showWLEDScan() {
    _screen = AppScreen::WLED_SCAN;
    _sprite.fillScreen(COL_BG);
    drawHeader("Scanning...");

    _sprite.setTextColor(COL_DIM);
    _sprite.setTextSize(1);
    _sprite.setCursor(10, CONTENT_Y + 60);
    _sprite.println("Looking for WLED");
    _sprite.setCursor(10, CONTENT_Y + 75);
    _sprite.println("devices on network");

    update();
}

void UI::showWLEDSelect(const std::vector<WLEDDevice>& devices) {
    _screen = AppScreen::WLED_SELECT;
    _itemCount = devices.size();
    _sprite.fillScreen(COL_BG);
    drawHeader("Select WLED");

    int y = CONTENT_Y;
    int maxVisible = CONTENT_H / ITEM_H;

    for (int i = _scrollOffset; i < _itemCount && (i - _scrollOffset) < maxVisible; i++) {
        bool selected = (i == _selectedIndex);
        drawMenuItem(y, devices[i].name + " (" + devices[i].ip + ")", selected);
        y += ITEM_H;
    }

    drawFooter("Scroll", "Select");
    update();
}

void UI::showMainStatus(const WLEDState& state, bool isWifiConnected) {
    _screen = AppScreen::MAIN_STATUS;

    const uint32_t bgColor       = 0x141416U;
    const uint32_t panelColor    = 0x1E1F22U;
    const uint32_t panelEdge     = 0x2A2C31U;
    const uint32_t textColor     = 0xF5F5F7U;
    const uint32_t labelColor    = 0x9A9CA3U;
    const uint32_t dimColor      = 0x676A72U;
    const uint32_t accentBlue    = 0x5AC8FAU;
    const uint32_t accentGreen   = 0x30D158U;
    const uint32_t accentRed     = 0xFF453AU;
    const uint32_t footerColor   = 0x18191CU;
    const uint32_t footerButton  = 0x25272BU;

    _sprite.fillScreen(bgColor);

    uint32_t liveColor = state.on
        ? _sprite.color888(state.r > 14 ? state.r : 14,
                           state.g > 14 ? state.g : 14,
                           state.b > 14 ? state.b : 14)
        : 0x4A4D55U;

    int brightnessPct = (state.brightness * 100 + 127) / 255;

    // Top status bar
    int statusY = 10;
    _sprite.setTextSize(1);
    _sprite.setTextColor(isWifiConnected ? accentGreen : accentRed);
    _sprite.setCursor(12, statusY);
    _sprite.print(isWifiConnected ? "WiFi" : "No WiFi");

    _sprite.setTextColor(state.reachable ? accentGreen : accentRed);
    const char* wledLabel = state.reachable ? "WLED" : "Offline";
    _sprite.setCursor(SCREEN_W - 12 - (int)strlen(wledLabel) * 6, statusY);
    _sprite.print(wledLabel);

    // Upper circular focus area
    const int cx = SCREEN_W / 2;
    const int cy = 94;
    const int outerR = 48;
    const int innerR = 35;
    const int glowR  = 58;

    _sprite.fillSmoothCircle(cx, cy, glowR, bgColor, bgColor);
    _sprite.fillSmoothCircle(cx, cy, outerR, panelEdge, bgColor);
    _sprite.fillSmoothCircle(cx, cy, outerR - 4, liveColor, panelEdge);
    _sprite.fillSmoothCircle(cx, cy, innerR, panelColor, liveColor);

    // Small top indicator dot inside circle
    _sprite.fillSmoothCircle(cx, cy - 22, 4, state.on ? accentGreen : dimColor, panelColor);

    char pctStr[8];
    snprintf(pctStr, sizeof(pctStr), "%d%%", brightnessPct);
    int pctLen = strlen(pctStr);
    _sprite.setTextColor(textColor);
    _sprite.setTextSize(3);
    _sprite.setCursor(cx - pctLen * 9, cy - 14);
    _sprite.print(pctStr);

    _sprite.setTextSize(1);
    _sprite.setTextColor(labelColor);
    const char* stateLabel = state.on ? "Brightness" : "Standby";
    _sprite.setCursor(cx - (int)strlen(stateLabel) * 3, cy + 18);
    _sprite.print(stateLabel);

    // Lower content panel
    int panelX = 12;
    int panelY = 156;
    int panelW = SCREEN_W - 24;
    int panelH = 108;
    _sprite.fillRoundRect(panelX, panelY, panelW, panelH, 18, panelColor);
    _sprite.drawRoundRect(panelX, panelY, panelW, panelH, 18, panelEdge);

    _sprite.setTextSize(1);
    _sprite.setTextColor(labelColor);
    _sprite.setCursor(panelX + 14, panelY + 14);
    _sprite.print(state.preset >= 0 ? "Current preset" : "Current effect");

    String statusStr = "Solid Color";
    if (state.preset >= 0) {
        for (const auto& p : wledClient.getPresets()) {
            if (p.first == state.preset) {
                statusStr = p.second;
                break;
            }
        }
    } else if (state.effect > 0) {
        if (state.effectName.length() > 0) {
            statusStr = state.effectName;
        } else {
            const auto& effects = wledClient.getEffects();
            if (state.effect < (int)effects.size()) {
                statusStr = effects[state.effect];
            }
        }
    }
    if (statusStr.length() == 0) {
        statusStr = "Solid Color";
    }
    if (statusStr.length() > 22) {
        statusStr = statusStr.substring(0, 21) + "…";
    }

    _sprite.setTextColor(textColor);
    _sprite.setTextSize(2);
    _sprite.setCursor(panelX + 14, panelY + 34);
    _sprite.print(statusStr);

    _sprite.setTextSize(1);
    _sprite.setTextColor(dimColor);
    _sprite.setCursor(panelX + 14, panelY + 64);
    if (!isWifiConnected || !state.reachable) {
        _sprite.print("Waiting for connection");
    } else if (state.preset >= 0) {
        _sprite.print("Effect resumes with preset");
    } else if (state.effect > 0) {
        _sprite.print("Live effect active");
    } else {
        _sprite.print("Manual color control");
    }

    // Color swatch chip
    int chipX = panelX + 14;
    int chipY = panelY + 82;
    int chipW = panelW - 28;
    int chipH = 14;
    _sprite.fillRoundRect(chipX, chipY, chipW, chipH, 7, 0x26282DU);
    _sprite.fillRoundRect(chipX + 2, chipY + 2, chipW - 4, chipH - 4, 6, liveColor);

    // Bottom two-button drawer
    int footerY = SCREEN_H - 44;
    int footerX = 10;
    int footerW = SCREEN_W - 20;
    int footerH = 30;
    _sprite.fillRoundRect(footerX, footerY, footerW, footerH, 15, footerColor);

    int gap = 8;
    int btnW = (footerW - gap - 8) / 2;
    int btnH = 22;
    int btnY = footerY + 4;
    int leftX = footerX + 4;
    int rightX = leftX + btnW + gap;

    _sprite.fillRoundRect(leftX, btnY, btnW, btnH, 11, footerButton);
    _sprite.fillRoundRect(rightX, btnY, btnW, btnH, 11, footerButton);

    _sprite.setTextSize(1);
    _sprite.setTextColor(textColor);
    _sprite.setCursor(leftX + btnW / 2 - 12, btnY + 7);
    _sprite.print("Menu");
    _sprite.setCursor(rightX + btnW / 2 - 21, btnY + 7);
    _sprite.print("Presets");

    update();
}

void UI::showMenu() {
    _screen = AppScreen::MENU;
    _itemCount = (int)MenuItem::MENU_COUNT;
    _sprite.fillScreen(COL_BG);
    drawHeader("Menu");

    const char* menuItems[] = {"Colors",  "Presets", "Effects", "HomeKit", "Settings"};
    int y = CONTENT_Y;
    for (int i = 0; i < _itemCount; i++) {
        drawMenuItem(y, menuItems[i], i == _selectedIndex);
        y += ITEM_H;
    }

    drawFooter("Scroll", "Back");
    update();
}

void UI::showColorPicker() {
    _screen = AppScreen::COLOR_PICKER;
    _sprite.fillScreen(COL_BG);
    drawHeader("Colors");

    // Predefined color grid
    struct PresetColor { uint8_t r, g, b; const char* name; };
    static const PresetColor colors[] = {
        {255, 0, 0, "Red"},       {0, 255, 0, "Green"},
        {0, 0, 255, "Blue"},      {255, 255, 0, "Yellow"},
        {255, 0, 255, "Purple"},  {0, 255, 255, "Cyan"},
        {255, 128, 0, "Orange"},  {255, 255, 255, "White"},
        {255, 180, 50, "Warm"},   {128, 0, 255, "Violet"},
        {0, 128, 255, "Sky"},     {255, 64, 128, "Pink"},
    };
    _itemCount = 12;

    int cols = 3;
    int swatchW = 45;
    int swatchH = 36;
    int gap = 8;
    int startX = (SCREEN_W - (cols * swatchW + (cols - 1) * gap)) / 2;
    int y = CONTENT_Y + 5;

    for (int i = 0; i < 12; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = startX + col * (swatchW + gap);
        int sy = y + row * (swatchH + gap + 10);

        bool selected = (i == _selectedIndex);
        if (selected) {
            _sprite.drawRect(x - 2, sy - 2, swatchW + 4, swatchH + 4, COL_ACCENT);
        }
        _sprite.fillRect(x, sy, swatchW, swatchH,
                         _sprite.color888(colors[i].r, colors[i].g, colors[i].b));

        _sprite.setTextColor(COL_DIM);
        _sprite.setTextSize(1);
        _sprite.setCursor(x, sy + swatchH + 2);
        _sprite.print(colors[i].name);
    }

    drawFooter("Scroll", "Apply");
    update();
}

void UI::showPresets(const std::vector<std::pair<int, String>>& presets) {
    _screen = AppScreen::PRESETS;
    _itemCount = presets.size();
    _sprite.fillScreen(COL_BG);
    drawHeader("Presets");

    int y = CONTENT_Y;
    int maxVisible = CONTENT_H / ITEM_H;

    if (_itemCount == 0) {
        _sprite.setTextColor(COL_DIM);
        _sprite.setTextSize(1);
        _sprite.setCursor(10, CONTENT_Y + 50);
        _sprite.println("No presets found");
    } else {
        for (int i = _scrollOffset; i < _itemCount && (i - _scrollOffset) < maxVisible; i++) {
            bool selected = (i == _selectedIndex);
            String label = String(presets[i].first) + ": " + presets[i].second;
            drawMenuItem(y, label, selected);
            y += ITEM_H;
        }
    }

    drawFooter("Scroll", "Back");
    update();
}

void UI::showEffects(const std::vector<String>& effects) {
    _screen = AppScreen::EFFECTS;
    _itemCount = effects.size();
    _sprite.fillScreen(COL_BG);
    drawHeader("Effects");

    int y = CONTENT_Y;
    int maxVisible = CONTENT_H / ITEM_H;

    for (int i = _scrollOffset; i < _itemCount && (i - _scrollOffset) < maxVisible; i++) {
        bool selected = (i == _selectedIndex);
        drawMenuItem(y, effects[i], selected);
        y += ITEM_H;
    }

    drawFooter("Scroll", "Back");
    update();
}

void UI::showHomeKitPairing(const String& setupCode, const String& setupURI) {
    _sprite.fillScreen(COL_BG);
    drawHeader("HomeKit");

    _sprite.setTextColor(COL_DIM);
    _sprite.setTextSize(1);
    _sprite.setCursor(10, CONTENT_Y + 5);
    _sprite.println("Scan QR or enter");
    _sprite.setCursor(10, CONTENT_Y + 17);
    _sprite.println("code in Home app:");

    // Display pairing code prominently
    _sprite.setTextColor(COL_ACCENT);
    _sprite.setTextSize(2);
    // Format code as XXX-XX-XXX
    String formatted = setupCode;
    if (setupCode.length() == 8) {
        formatted = setupCode.substring(0, 3) + "-" + setupCode.substring(3, 5) + "-" + setupCode.substring(5, 8);
    }
    int tw = formatted.length() * 12;
    _sprite.setCursor((SCREEN_W - tw) / 2, CONTENT_Y + 40);
    _sprite.println(formatted);

    // Push sprite first
    _sprite.pushSprite(_tft, 0, 0);

    // Draw QR code with the proper X-HM:// URI
    int qrSize = 100;
    int qrX = (SCREEN_W - qrSize) / 2;
    int qrY = CONTENT_Y + 70;
    _tft->qrcode(setupURI.c_str(), qrX, qrY, qrSize, 4);

    // Draw footer directly on display
    _tft->setTextColor(COL_DIM);
    _tft->setTextSize(1);
    _tft->setCursor(10, SCREEN_H - FOOTER_H + 8);
    _tft->print("[Back]");
}

void UI::showError(const String& message) {
    _sprite.fillScreen(COL_BG);
    drawHeader("Error");
    _sprite.setTextColor(COL_WARNING);
    _sprite.setTextSize(1);
    _sprite.setCursor(10, CONTENT_Y + 50);
    _sprite.println(message);
    drawFooter("", "OK");
    update();
}

void UI::showMessage(const String& title, const String& message) {
    _sprite.fillScreen(COL_BG);
    drawHeader(title);
    _sprite.setTextColor(COL_TEXT);
    _sprite.setTextSize(1);
    _sprite.setCursor(10, CONTENT_Y + 40);
    _sprite.println(message);
    update();
   }

void UI::showRecovering(const String& msg) {
    _screen = AppScreen::RECOVERING;
    _sprite.fillScreen(0x1C1C1EU);

    int cx = SCREEN_W / 2;
    int cy = CONTENT_Y + 40;
    uint32_t warnColor = 0xFFAA00U;
    _sprite.drawTriangle(cx, cy - 24, cx - 24, cy + 20, cx + 24, cy + 20, warnColor);
    _sprite.fillCircle(cx, cy - 4, 3, warnColor);
    _sprite.drawLine(cx, cy + 2, cx, cy + 10, warnColor);

    _sprite.setTextColor(COL_TEXT);
    _sprite.setTextSize(1);
    int msgW = 10 * 6;
    _sprite.setCursor((SCREEN_W - msgW) / 2, cy + 36);
    _sprite.print("Recovering");

    _sprite.setTextColor(COL_DIM);
    _sprite.setCursor((SCREEN_W - msg.length() * 6) / 2, cy + 54);
    _sprite.print(msg);

    _sprite.setTextColor(COL_DIM);
    _sprite.setTextSize(1);
    _sprite.setCursor(10, SCREEN_H - FOOTER_H + 8);
    _sprite.print("[Back] Reconfigure");

    update();
}

void UI::showLoading() {
    _screen = AppScreen::LOADING;
    _sprite.fillScreen(0x1C1C1EU);

      // Center "Loading presets..." text
    _sprite.setTextColor(COL_TEXT);
    _sprite.setTextSize(2);
    int tw = 18 * 12;
    _sprite.setCursor((SCREEN_W - tw) / 2, CONTENT_Y + 50);
    _sprite.print("Loading presets");

      // Subtitle
    _sprite.setTextColor(COL_DIM);
    _sprite.setTextSize(1);
    String sub = "Please wait...";
    _sprite.setCursor((SCREEN_W - sub.length() * 6) / 2, CONTENT_Y + 75);
    _sprite.print(sub);

    update();
    }

bool UI::handleLeft(ButtonEvent event) {
    if (event == ButtonEvent::NONE) return false;
    resetActivityTimer();

    if (event == ButtonEvent::LONG_PRESS) {
        // Long press left = back to main status from any submenu
        if (_screen != AppScreen::MAIN_STATUS) {
            _selectedIndex = 0;
            _scrollOffset = 0;
            return true;  // Caller should switch to main status
        }
        return false;
    }

    // Short press = scroll/navigate
    switch (_screen) {
        case AppScreen::MENU:
        case AppScreen::WLED_SELECT:
        case AppScreen::PRESETS:
        case AppScreen::EFFECTS:
            _selectedIndex++;
            if (_selectedIndex >= _itemCount) _selectedIndex = 0;
            // Adjust scroll
            {
                int maxVisible = CONTENT_H / ITEM_H;
                if (_selectedIndex >= _scrollOffset + maxVisible) {
                    _scrollOffset = _selectedIndex - maxVisible + 1;
                } else if (_selectedIndex < _scrollOffset) {
                    _scrollOffset = _selectedIndex;
                }
            }
            return true;

        case AppScreen::COLOR_PICKER:
            _selectedIndex++;
            if (_selectedIndex >= _itemCount) _selectedIndex = 0;
            return true;

        case AppScreen::BRIGHTNESS:
            // Decrease brightness
            return true;

        default:
            return false;
    }
}

bool UI::handleRight(ButtonEvent event) {
    if (event == ButtonEvent::NONE) return false;
    resetActivityTimer();

    // Right press = select/confirm/toggle
    return true;  // Signal that right button was pressed; main handles the action
}

bool UI::handleRotation(int8_t direction) {
    if (direction == 0) return false;
    resetActivityTimer();

    if (_itemCount <= 0) return false;

    switch (_screen) {
        case AppScreen::MENU:
        case AppScreen::WLED_SELECT:
        case AppScreen::PRESETS:
        case AppScreen::EFFECTS: {
            _selectedIndex += direction;
            if (_selectedIndex >= _itemCount) _selectedIndex = 0;
            else if (_selectedIndex < 0) _selectedIndex = _itemCount - 1;

            int maxVisible = CONTENT_H / ITEM_H;
            if (_selectedIndex >= _scrollOffset + maxVisible) {
                _scrollOffset = _selectedIndex - maxVisible + 1;
            } else if (_selectedIndex < _scrollOffset) {
                _scrollOffset = _selectedIndex;
            }
            return true;
        }

        case AppScreen::COLOR_PICKER:
            _selectedIndex += direction;
            if (_selectedIndex >= _itemCount) _selectedIndex = 0;
            else if (_selectedIndex < 0) _selectedIndex = _itemCount - 1;
            return true;

        default:
            return false;
    }
}

void UI::resetActivityTimer() {
    _lastActivity = millis();
}

bool UI::shouldDim() const {
    return (millis() - _lastActivity) > DIM_TIMEOUT_MS;
}

// Private helpers

void UI::drawHeader(const String& title) {
    _sprite.fillRect(0, 0, SCREEN_W, HEADER_H, COL_HEADER);
    _sprite.setTextColor(COL_ACCENT);
    _sprite.setTextSize(2);
    _sprite.setCursor(8, 7);
    _sprite.print(title);
}

void UI::drawFooter(const String& leftLabel, const String& rightLabel) {
    int y = SCREEN_H - FOOTER_H;
    _sprite.drawFastHLine(0, y, SCREEN_W, COL_DIM);
    _sprite.setTextColor(COL_DIM);
    _sprite.setTextSize(1);
    if (leftLabel.length() > 0) {
        _sprite.setCursor(8, y + 8);
        _sprite.print("[" + leftLabel + "]");
    }
    if (rightLabel.length() > 0) {
        int tw = rightLabel.length() * 6 + 12;
        _sprite.setCursor(SCREEN_W - tw, y + 8);
        _sprite.print("[" + rightLabel + "]");
    }
}

void UI::drawMenuItem(int y, const String& text, bool selected) {
    if (selected) {
        _sprite.fillRect(0, y, SCREEN_W, ITEM_H - 2, COL_SELECT);
        _sprite.setTextColor(COL_ACCENT);
    } else {
        _sprite.setTextColor(COL_TEXT);
    }
    _sprite.setTextSize(1);
    _sprite.setCursor(10, y + 8);
    _sprite.print(text);
    if (selected) {
        _sprite.fillRect(0, y, 3, ITEM_H - 2, COL_ACCENT);
    }
}

void UI::drawProgressBar(int x, int y, int w, int h, uint8_t value, uint32_t color) {
    _sprite.drawRect(x, y, w, h, COL_DIM);
    int fillW = ((w - 2) * value) / 255;
    if (fillW > 0) {
        _sprite.fillRect(x + 1, y + 1, fillW, h - 2, color);
    }
}

void UI::drawColorSwatch(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    _sprite.fillRect(x, y, w, h, _sprite.color888(r, g, b));
    _sprite.drawRect(x, y, w, h, COL_DIM);
}
