#include <Arduino.h>
#include <WiFi.h>

#include "lgfx_setup.h"
#include "config_store.h"
#include "wifi_setup.h"
#include "wled_discovery.h"
#include "wled_client.h"
#include "button_handler.h"
#include "ui.h"
#include "config_server.h"
#include "homekit_bridge.h"

// Hardware pins
// Bare 5-pin rotary encoder wiring:
//   Rotary side:       Switch side:
//     [CLK] [GND] [DT]     [SW] [GND]
//
//   - CLK -> ENCODER_CLK_PIN (GPIO 21)
//   - DT  -> ENCODER_DT_PIN  (GPIO 22)
//   - SW  -> ENCODER_SW_PIN  (GPIO 23)
//   - Connect both GND pins to board ground


#define BUTTON_LEFT_PIN  0
#define BUTTON_RIGHT_PIN 14
#define POWER_PIN        15
#define LED_PIN          2

#define BUTTON_MENU_PIN   0
#define BUTTON_BACK_PIN   14
#define ENCODER_CLK_PIN   1
#define ENCODER_DT_PIN    17
#define ENCODER_SW_PIN    10
#define POWER_PIN         15
#define LED_PIN           2

// Display instance
static LGFX tft;

// Application state
enum class AppState {
    BOOT,
    WIFI_SETUP,
    WIFI_CONNECTED,
    WLED_SCAN,
    WLED_SELECT,
    RUNNING
};

static AppState appState = AppState::BOOT;
static uint32_t lastPoll = 0;
static constexpr uint32_t POLL_INTERVAL_MS = 2000;

// Screensaver state machine
enum class ScreenState {
    ON,         // Full brightness, 4 sec
    DIM,        // Dimmed, 4 sec
    SAVER,      // Raindrop animation, 5 sec
    OFF         // Display off
};
static ScreenState screenState = ScreenState::ON;
static uint32_t screenStateStart = 0;
static constexpr uint32_t SCREEN_ON_MS    = 10000;
static constexpr uint32_t SCREEN_DIM_MS   = 10000;
static constexpr uint32_t SCREEN_SAVER_MS = 10000;

// Cat screensaver animation state
struct FallingCat {
    int16_t x, y;
    int16_t speed;
    uint16_t color;
    uint8_t face;  // which cat face to draw
    bool active;
};
static constexpr int MAX_DROPS = 8;
static FallingCat drops[MAX_DROPS];
static uint32_t lastDropFrame = 0;
static bool saverInitialized = false;

// 16x16 cat face bitmaps (1 bit per pixel, MSB first)
// Cat 1: round face, pointy ears, happy eyes
static const uint8_t cat1[] PROGMEM = {
    0b00000000, 0b00000000,
    0b00100000, 0b00000100,
    0b01110000, 0b00001110,
    0b01111000, 0b00011110,
    0b01111111, 0b11111110,
    0b00111111, 0b11111100,
    0b00111111, 0b11111100,
    0b00111001, 0b10011100,
    0b00111111, 0b11111100,
    0b00111111, 0b11111100,
    0b00111010, 0b01011100,
    0b00111100, 0b00111100,
    0b00011111, 0b11111000,
    0b00001111, 0b11110000,
    0b00000111, 0b11100000,
    0b00000000, 0b00000000,
};

// Cat 2: wider face, ^ eyes
static const uint8_t cat2[] PROGMEM = {
    0b00000000, 0b00000000,
    0b01000000, 0b00000010,
    0b01100000, 0b00000110,
    0b01110000, 0b00001110,
    0b01111111, 0b11111110,
    0b00111111, 0b11111100,
    0b00111111, 0b11111100,
    0b00110101, 0b01011100,
    0b00111001, 0b10011100,
    0b00111111, 0b11111100,
    0b00111010, 0b01011100,
    0b00111101, 0b10111100,
    0b00011111, 0b11111000,
    0b00001111, 0b11110000,
    0b00000111, 0b11100000,
    0b00000000, 0b00000000,
};

// Cat 3: sleepy/content face
static const uint8_t cat3[] PROGMEM = {
    0b00000000, 0b00000000,
    0b00100000, 0b00000100,
    0b01110000, 0b00001110,
    0b01111000, 0b00011110,
    0b01111111, 0b11111110,
    0b00111111, 0b11111100,
    0b00111111, 0b11111100,
    0b00111111, 0b11111100,
    0b00110110, 0b01101100,
    0b00111111, 0b11111100,
    0b00111010, 0b01011100,
    0b00111100, 0b00111100,
    0b00011111, 0b11111000,
    0b00001111, 0b11110000,
    0b00000111, 0b11100000,
    0b00000000, 0b00000000,
};

// Cat 4: surprised face, O eyes
static const uint8_t cat4[] PROGMEM = {
    0b00000000, 0b00000000,
    0b01000000, 0b00000010,
    0b01100000, 0b00000110,
    0b01110000, 0b00001110,
    0b01111111, 0b11111110,
    0b00111111, 0b11111100,
    0b00110110, 0b01101100,
    0b00110110, 0b01101100,
    0b00110110, 0b01101100,
    0b00111111, 0b11111100,
    0b00111100, 0b00111100,
    0b00111110, 0b01111100,
    0b00011111, 0b11111000,
    0b00001111, 0b11110000,
    0b00000111, 0b11100000,
    0b00000000, 0b00000000,
};

static const uint8_t* catBitmaps[] = { cat1, cat2, cat3, cat4 };
static constexpr int CAT_W = 16;
static constexpr int CAT_H = 16;
static constexpr int NUM_CAT_FACES = 4;

// Predefined colors for color picker
struct PresetColor { uint8_t r, g, b; };
static const PresetColor presetColors[] = {
    {255, 0, 0},    {0, 255, 0},    {0, 0, 255},    {255, 255, 0},
    {255, 0, 255},  {0, 255, 255},  {255, 128, 0},  {255, 255, 255},
    {255, 180, 50}, {128, 0, 255},  {0, 128, 255},  {255, 64, 128},
};

// Forward declarations
void transitionTo(AppState newState);
void handleRunningState();
void handleButtons();
void updateScreensaver();
void initRaindrops();
void drawRaindrops();

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[BOOT] WLED Remote starting...");

    // Enable display power (required for T-Display S3)
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);

    // Initialize display
    tft.init();
    tft.setRotation(0);  // Portrait, buttons at bottom
    tft.setBrightness(200);
    tft.fillScreen(TFT_BLACK);

    // Initialize UI with display
    ui.begin(&tft);
    ui.showBoot();

    // Initialize buttons and rotary encoder
    buttons.begin(BUTTON_MENU_PIN, BUTTON_BACK_PIN,
                  ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);

    // Initialize config store (NVS)
    configStore.begin();

    // Onboard LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    delay(1500);  // Show boot screen

    // Initialize screensaver timer
    screenStateStart = millis();

    // Transition to WiFi setup
    transitionTo(AppState::WIFI_SETUP);
}

void loop() {
    // Always update buttons
    buttons.update();

    // Handle screensaver: button press wakes display
    if (buttons.anyActivity()) {
        ui.resetActivityTimer();
        if (screenState != ScreenState::ON) {
            screenState = ScreenState::ON;
            screenStateStart = millis();
            tft.setBrightness(200);
            saverInitialized = false;
            // Redraw current screen
            if (appState == AppState::RUNNING) {
                ui.showMainStatus(wledClient.getState());
            }
        } else {
            screenStateStart = millis();  // Reset timer on activity
        }
    }

    // Update screensaver state machine
    updateScreensaver();

    // State machine
    switch (appState) {
        case AppState::BOOT:
            break;

        case AppState::WIFI_SETUP:
            // WiFiManager blocks during portal, so we get here after it returns
            break;

        case AppState::WIFI_CONNECTED:
            // Config server running, handle requests
            configServer.handleClient();
            handleButtons();
            break;

        case AppState::WLED_SCAN:
            break;

        case AppState::WLED_SELECT:
            configServer.handleClient();
            handleButtons();
            break;

        case AppState::RUNNING:
            handleRunningState();
            handleButtons();
            break;
    }
}

void transitionTo(AppState newState) {
    appState = newState;

    switch (newState) {
        case AppState::WIFI_SETUP: {
            Serial.println("[STATE] WiFi Setup");
            if (wifiSetup.hasSavedCredentials()) {
                ui.showConnecting();
            } else {
                ui.showAPMode("WLED-Bridge");
            }

            bool connected = wifiSetup.begin("WLED-Bridge", 180);
            if (connected) {
                transitionTo(AppState::WIFI_CONNECTED);
            } else {
                Serial.println("[WiFi] Portal timeout, restarting...");
                ESP.restart();
            }
            break;
        }

        case AppState::WIFI_CONNECTED: {
            Serial.println("[STATE] WiFi Connected");
            String ip = wifiSetup.getIPAddress();
            Serial.printf("[WiFi] IP: %s\n", ip.c_str());
            ui.showIP(ip);

            // Start mDNS
            wledDiscovery.begin();

            // Start config web server
            configServer.begin();

            // Check if WLED is already configured
            if (configStore.hasWLEDConfig()) {
                String host = configStore.getWLEDHost();
                uint16_t port = configStore.getWLEDPort();
                wledClient.setHost(host, port);
                Serial.printf("[WLED] Configured: %s:%d\n", host.c_str(), port);

                // Try to fetch initial state
                if (wledClient.fetchState()) {
                    wledClient.fetchEffects();
                    wledClient.fetchPresets();

                    // Start HomeKit bridge
                    homeKitBridge.begin(configStore.getHomeKitCode().c_str());

                    transitionTo(AppState::RUNNING);
                } else {
                    // WLED not reachable, but config exists - still go to running
                    homeKitBridge.begin(configStore.getHomeKitCode().c_str());
                    transitionTo(AppState::RUNNING);
                }
            } else {
                // No WLED config - scan for devices
                transitionTo(AppState::WLED_SCAN);
            }
            break;
        }

        case AppState::WLED_SCAN: {
            Serial.println("[STATE] WLED Scan");
            ui.showWLEDScan();

            int found = wledDiscovery.scan();
            if (found > 0) {
                transitionTo(AppState::WLED_SELECT);
            } else {
                // No devices found - show IP for manual config via web UI
                ui.showIP(wifiSetup.getIPAddress());
                ui.showMessage("No WLED Found", "Use web UI to\nconfigure manually:\n" + wifiSetup.getIPAddress());
                appState = AppState::WIFI_CONNECTED;
            }
            break;
        }

        case AppState::WLED_SELECT: {
            Serial.println("[STATE] WLED Select");
            ui.showWLEDSelect(wledDiscovery.getDevices());
            break;
        }

        case AppState::RUNNING: {
            Serial.println("[STATE] Running");
            ui.showMainStatus(wledClient.getState());
            lastPoll = millis();
            break;
        }

        default:
            break;
    }
}

void handleRunningState() {
    // Poll WLED state periodically
    uint32_t now = millis();
    if (now - lastPoll >= POLL_INTERVAL_MS) {
        lastPoll = now;
        if (wledClient.fetchState()) {
            // Sync HomeKit
            homeKitBridge.syncState(wledClient.getState());

            // Update display if on main status screen
            if (ui.getCurrentScreen() == AppScreen::MAIN_STATUS) {
                ui.showMainStatus(wledClient.getState());
            }
        }
    }

    // Handle HomeKit
    homeKitBridge.poll();

    // Config server still running for web access
    configServer.handleClient();
}

void handleButtons() {
    ButtonEvent menuEvt = buttons.getLeftEvent();
    ButtonEvent backEvt = buttons.getRightEvent();
    ButtonEvent encoderBtnEvt = buttons.getEncoderButtonEvent();
    int8_t encoderMove = buttons.getEncoderMovement();

    if (menuEvt == ButtonEvent::NONE && backEvt == ButtonEvent::NONE &&
        encoderBtnEvt == ButtonEvent::NONE && encoderMove == 0) return;

    AppScreen screen = ui.getCurrentScreen();

    if (appState == AppState::RUNNING && screen == AppScreen::MAIN_STATUS) {
        if (encoderMove != 0) {
            int bri = wledClient.getState().brightness;
            bri = constrain(bri + encoderMove * 25, 0, 255);
            wledClient.setBrightness(bri);
            delay(50);
            wledClient.fetchState();
            ui.showMainStatus(wledClient.getState());
            return;
        }

        if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
            wledClient.togglePower();
            delay(100);
            wledClient.fetchState();
            ui.showMainStatus(wledClient.getState());
            return;
        }

        if (menuEvt == ButtonEvent::SHORT_PRESS) {
            ui.showMenu();
            return;
        }
    }

    if ((backEvt == ButtonEvent::SHORT_PRESS || backEvt == ButtonEvent::LONG_PRESS) &&
        screen != AppScreen::MAIN_STATUS && appState == AppState::RUNNING) {
        ui.showMainStatus(wledClient.getState());
        return;
    }

    switch (screen) {
        case AppScreen::SHOW_IP:
            if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
                if (configStore.hasWLEDConfig()) {
                    transitionTo(AppState::RUNNING);
                } else {
                    transitionTo(AppState::WLED_SCAN);
                }
            }
            break;

        case AppScreen::WLED_SELECT:
            if (menuEvt == ButtonEvent::SHORT_PRESS) {
                ui.handleLeft(menuEvt);
                ui.showWLEDSelect(wledDiscovery.getDevices());
            }
            if (encoderMove != 0 && ui.handleRotation(encoderMove)) {
                ui.showWLEDSelect(wledDiscovery.getDevices());
            }
            if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
                int idx = ui.getSelectedIndex();
                const WLEDDevice* device = wledDiscovery.getDevice(idx);
                if (device) {
                    configStore.setWLEDHost(device->ip);
                    configStore.setWLEDPort(device->port);
                    wledClient.setHost(device->ip, device->port);
                    wledClient.fetchState();
                    wledClient.fetchEffects();
                    wledClient.fetchPresets();
                    homeKitBridge.begin(configStore.getHomeKitCode().c_str());
                    transitionTo(AppState::RUNNING);
                }
            }
            break;

        case AppScreen::MAIN_STATUS:
            if (menuEvt == ButtonEvent::SHORT_PRESS) {
                ui.showMenu();
            }
            break;

        case AppScreen::MENU:
            if (menuEvt == ButtonEvent::SHORT_PRESS) {
                ui.handleLeft(menuEvt);
                ui.showMenu();
            }
            if (encoderMove != 0 && ui.handleRotation(encoderMove)) {
                ui.showMenu();
            }
            if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
                int idx = ui.getSelectedIndex();
                switch ((MenuItem)idx) {
                    case MenuItem::COLORS:
                        ui.showColorPicker();
                        break;
                    case MenuItem::BRIGHTNESS:
                        ui.showBrightness(wledClient.getState().brightness);
                        break;
                    case MenuItem::PRESETS:
                        ui.showPresets(wledClient.getPresets());
                        break;
                    case MenuItem::EFFECTS:
                        ui.showEffects(wledClient.getEffects());
                        break;
                    case MenuItem::HOMEKIT:
                        ui.showHomeKitPairing(configStore.getHomeKitCode(), homeKitBridge.getSetupURI());
                        break;
                    case MenuItem::SETTINGS:
                        ui.showIP(wifiSetup.getIPAddress());
                        break;
                    default:
                        break;
                }
            }
            break;

        case AppScreen::COLOR_PICKER:
            if (menuEvt == ButtonEvent::SHORT_PRESS) {
                ui.handleLeft(menuEvt);
                ui.showColorPicker();
            }
            if (encoderMove != 0 && ui.handleRotation(encoderMove)) {
                ui.showColorPicker();
            }
            if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
                int idx = ui.getSelectedIndex();
                if (idx >= 0 && idx < 12) {
                    wledClient.setColor(presetColors[idx].r, presetColors[idx].g, presetColors[idx].b);
                    delay(100);
                    wledClient.fetchState();
                    ui.showMainStatus(wledClient.getState());
                }
            }
            break;

        case AppScreen::BRIGHTNESS:
            if (encoderMove != 0) {
                int bri = wledClient.getState().brightness;
                bri = constrain(bri + encoderMove * 25, 0, 255);
                wledClient.setBrightness(bri);
                delay(50);
                wledClient.fetchState();
                ui.showBrightness(wledClient.getState().brightness);
            }
            break;

        case AppScreen::PRESETS:
            if (menuEvt == ButtonEvent::SHORT_PRESS) {
                ui.handleLeft(menuEvt);
                ui.showPresets(wledClient.getPresets());
            }
            if (encoderMove != 0 && ui.handleRotation(encoderMove)) {
                ui.showPresets(wledClient.getPresets());
            }
            if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
                int idx = ui.getSelectedIndex();
                const auto& presets = wledClient.getPresets();
                if (idx >= 0 && idx < (int)presets.size()) {
                    wledClient.setPreset(presets[idx].first);
                    delay(200);
                    wledClient.fetchState();
                    ui.showMainStatus(wledClient.getState());
                }
            }
            break;

        case AppScreen::EFFECTS:
            if (menuEvt == ButtonEvent::SHORT_PRESS) {
                ui.handleLeft(menuEvt);
                ui.showEffects(wledClient.getEffects());
            }
            if (encoderMove != 0 && ui.handleRotation(encoderMove)) {
                ui.showEffects(wledClient.getEffects());
            }
            if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
                int idx = ui.getSelectedIndex();
                if (idx >= 0 && idx < (int)wledClient.getEffects().size()) {
                    wledClient.setEffect(idx);
                    delay(200);
                    wledClient.fetchState();
                    ui.showMainStatus(wledClient.getState());
                }
            }
            break;

        default:
            break;
    }
}

void initRaindrops() {
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = random(2, 170 - CAT_W);
        drops[i].y = random(-320, -20);
        drops[i].speed = random(2, 5);
        drops[i].face = random(0, NUM_CAT_FACES);
        // Pastel colors for cute cats
        uint8_t r = random(180, 255);
        uint8_t g = random(100, 230);
        uint8_t b = random(180, 255);
        drops[i].color = tft.color565(r, g, b);
        drops[i].active = true;
    }
}

static void drawCatBitmap(int16_t x, int16_t y, const uint8_t* bmp, uint16_t color) {
    for (int row = 0; row < CAT_H; row++) {
        uint8_t b0 = pgm_read_byte(&bmp[row * 2]);
        uint8_t b1 = pgm_read_byte(&bmp[row * 2 + 1]);
        uint16_t rowBits = ((uint16_t)b0 << 8) | b1;
        for (int col = 0; col < CAT_W; col++) {
            if (rowBits & (0x8000 >> col)) {
                tft.drawPixel(x + col, y + row, color);
            }
        }
    }
}

void drawRaindrops() {
    uint32_t now = millis();
    if (now - lastDropFrame < 50) return;  // ~20 fps
    lastDropFrame = now;

    tft.fillScreen(TFT_BLACK);

    for (int i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].active) continue;

        drops[i].y += drops[i].speed;

        // Draw cat face bitmap
        drawCatBitmap(drops[i].x, drops[i].y, catBitmaps[drops[i].face], drops[i].color);

        // Reset when off screen
        if (drops[i].y > 320) {
            drops[i].x = random(2, 170 - CAT_W);
            drops[i].y = random(-30, -16);
            drops[i].speed = random(2, 5);
            drops[i].face = random(0, NUM_CAT_FACES);
            uint8_t r = random(180, 255);
            uint8_t g = random(100, 230);
            uint8_t b = random(180, 255);
            drops[i].color = tft.color565(r, g, b);
        }
    }
}

void updateScreensaver() {
    uint32_t elapsed = millis() - screenStateStart;

    switch (screenState) {
        case ScreenState::ON:
            if (elapsed >= SCREEN_ON_MS) {
                screenState = ScreenState::DIM;
                screenStateStart = millis();
                tft.setBrightness(30);
            }
            break;

        case ScreenState::DIM:
            if (elapsed >= SCREEN_DIM_MS) {
                screenState = ScreenState::SAVER;
                screenStateStart = millis();
                tft.setBrightness(80);
                saverInitialized = false;
            }
            break;

        case ScreenState::SAVER:
            if (!saverInitialized) {
                initRaindrops();
                saverInitialized = true;
            }
            drawRaindrops();
            if (elapsed >= SCREEN_SAVER_MS) {
                screenState = ScreenState::OFF;
                screenStateStart = millis();
                tft.setBrightness(0);
                tft.fillScreen(TFT_BLACK);
            }
            break;

        case ScreenState::OFF:
            // Display off, waiting for button press
            break;
    }
}
