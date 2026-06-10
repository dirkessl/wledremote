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

#define BUTTON_MENU_PIN   BUTTON_LEFT_PIN
#define BUTTON_BACK_PIN   BUTTON_RIGHT_PIN
#define ENCODER_CLK_PIN   21
#define ENCODER_DT_PIN    22
#define ENCODER_SW_PIN    23
// Display instance
static LGFX tft;

// Application state
enum class AppState {
    BOOT,
    WIFI_CONNECTING,
    CAPTIVE_PORTAL,
    WLED_CONNECTING,
    RUNNING,
    RECOVERING,
    WLED_SCAN,
    WLED_SELECT,
    LOADING
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
static constexpr uint32_t SCREEN_ON_MS    = 60000;
static constexpr uint32_t SCREEN_DIM_MS   = 60000;
static constexpr uint32_t SCREEN_SAVER_MS = 60000;

// Cat screensaver animation state
// Ambient screensaver state
struct AmbientBlob {
    float x, y;
    float dx, dy;
    float radius;
    uint16_t color;
};
static constexpr int NUM_BLOBS = 5;
static AmbientBlob blobs[NUM_BLOBS];
static uint32_t lastBlobFrame = 0;
static bool saverInitialized = false;

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
void handleRecoveringState();
void handleLoadingState();
void handleButtons();
void updateScreensaver();
void initAmbientSaver();
void drawAmbientSaver();

// WiFi reconnect state
static uint8_t _wifiLostRetry = 0;
static String _recoverMsg = "";
static uint32_t _recoverTimer = 0;

uint32_t getPollInterval() {
    if (appState != AppState::RUNNING) return 5000;
    switch (screenState) {
        case ScreenState::ON: return 1000;
        case ScreenState::DIM: return 2000;
        case ScreenState::SAVER: return 4000;
        case ScreenState::OFF: return 10000;
    }
    return 2000;
}

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
    
    // Load cached wled data
    wledClient.loadCache();

    // Onboard LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    delay(1500);  // Show boot screen

    // Initialize screensaver timer
    screenStateStart = millis();

    // Transition to WiFi setup
    transitionTo(AppState::WIFI_CONNECTING);
}

void loop() {
      // Always update buttons
    buttons.update();

     // WiFi disconnect detection (only while RUNNING)
    if (appState == AppState::RUNNING && !wifiSetup.isConnected()) {
         Serial.println("[WiFi] Connection lost!");
        _recoverMsg = "Lost WiFi connection";
        ui.showRecovering(_recoverMsg);
        transitionTo(AppState::RECOVERING);
         return;
         }

    // WLED disconnect detection (only while RUNNING)
    if (appState == AppState::RUNNING && !wledClient.isReachable() && (millis() - lastPoll > getPollInterval() * 2)) {
         Serial.println("[WLED] Connection lost!");
        _recoverMsg = "Lost WLED connection";
        ui.showRecovering(_recoverMsg);
        transitionTo(AppState::RECOVERING);
         return;
    }

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
                // Immediate poll on wake
                wledClient.fetchState();
                lastPoll = millis();
                ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
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

        case AppState::WIFI_CONNECTING:
            // Config server running if connected, but WiFi manager handles portal
            break;

        case AppState::CAPTIVE_PORTAL:
            // Config server handles portal
            break;

        case AppState::WLED_CONNECTING:
            break;

        case AppState::WLED_SCAN:
            break;

         case AppState::WLED_SELECT:
             configServer.handleClient();
             handleButtons();
             break;

          case AppState::RECOVERING:
             handleRecoveringState();
             handleButtons();
             break;

          case AppState::LOADING:
             handleLoadingState();
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
        case AppState::WIFI_CONNECTING: {
            Serial.println("[STATE] WiFi Connecting");
            if (wifiSetup.hasSavedCredentials()) {
                ui.showWifiConnecting();
            } else {
                ui.showAPMode("WLED-Bridge");
                transitionTo(AppState::CAPTIVE_PORTAL);
                return;
            }

            bool connected = wifiSetup.begin("WLED-Bridge", 180);
            if (connected) {
                // Start mDNS and web server
                wledDiscovery.begin();
                configServer.begin();
                transitionTo(AppState::WLED_CONNECTING);
            } else {
                Serial.println("[WiFi] Connection failed, showing portal");
                ui.showAPMode("WLED-Bridge");
                transitionTo(AppState::CAPTIVE_PORTAL);
            }
            break;
        }

        case AppState::CAPTIVE_PORTAL: {
            bool connected = wifiSetup.begin("WLED-Bridge", 180); // blocks until portal exits
            if (connected) {
                wledDiscovery.begin();
                configServer.begin();
                transitionTo(AppState::WLED_CONNECTING);
            } else {
                ESP.restart();
            }
            break;
        }

         case AppState::WLED_CONNECTING: {
             Serial.println("[STATE] WLED Connecting");
             ui.showWledConnecting();

              // Check if WLED is already configured
             if (configStore.hasWLEDConfig()) {
                 String host = configStore.getWLEDHost();
                 uint16_t port = configStore.getWLEDPort();
                 wledClient.setHost(host, port);

                  // Try to fetch initial state
                 if (wledClient.fetchState()) {
                     homeKitBridge.begin(configStore.getHomeKitCode().c_str());
                     transitionTo(AppState::LOADING);
                   } else {
                      // WLED not reachable, go to recovering
                     _recoverMsg = "WLED unreachable";
                     homeKitBridge.begin(configStore.getHomeKitCode().c_str());
                     transitionTo(AppState::RECOVERING);
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
                appState = AppState::RUNNING;
            }
            break;
        }

        case AppState::WLED_SELECT: {
             Serial.println("[STATE] WLED Select");
             ui.showWLEDSelect(wledDiscovery.getDevices());
              break;
           }

          case AppState::RECOVERING: {
              Serial.println("[STATE] Recovering");
              ui.showRecovering(_recoverMsg);
              _recoverTimer = millis();
               break;
             }

          case AppState::LOADING: {
              Serial.println("[STATE] Loading");
              if (wledClient.getFetchStatus() != FetchStatus::IN_PROGRESS) {
                   // Start async fetch
                  wledClient.clearFetchStatus();
                  wledClient.asyncFetchAll();
                  }
              ui.showLoading();
               break;
             }

          case AppState::RUNNING: {
            Serial.println("[STATE] Running");
            ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
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
    if (now - lastPoll >= getPollInterval()) {
        lastPoll = now;
        if (wledClient.fetchState()) {
            // Sync HomeKit
            homeKitBridge.syncState(wledClient.getState());

            // Update display if on main status screen
            if (ui.getCurrentScreen() == AppScreen::MAIN_STATUS && screenState != ScreenState::OFF) {
                ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
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
                wledClient.fetchState();
                ui.showMainStatus(wledClient.getState());
                return;
            }

        if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
            wledClient.togglePower();
            wledClient.fetchState();
            ui.showMainStatus(wledClient.getState());
            return;
        }

        if (menuEvt == ButtonEvent::SHORT_PRESS) {
            ui.showMenu();
            return;
        }

        if (backEvt == ButtonEvent::SHORT_PRESS) {
            ui.showPresets(wledClient.getPresets());
            return;
        }
    }

    // Back from RECOVERING → reconfigure (AP portal)
   if ((backEvt == ButtonEvent::SHORT_PRESS || backEvt == ButtonEvent::LONG_PRESS) &&
       screen == AppScreen::RECOVERING) {
         transitionTo(AppState::CAPTIVE_PORTAL);
          return;
        }

      // Back from LOADING → skip back to running
    if ((backEvt == ButtonEvent::SHORT_PRESS || backEvt == ButtonEvent::LONG_PRESS) &&
       (screen == AppScreen::LOADING)) {
         wledClient.clearFetchStatus();
        transitionTo(AppState::RUNNING);
          return;
        }

      // Back from submenu → main status
    if ((backEvt == ButtonEvent::SHORT_PRESS || backEvt == ButtonEvent::LONG_PRESS) &&
       screen != AppScreen::MAIN_STATUS && appState == AppState::RUNNING) {
        ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
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
                      homeKitBridge.begin(configStore.getHomeKitCode().c_str());
                       // Start async fetch of effects + presets, then go to loading screen
                     transitionTo(AppState::LOADING);
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
                        wledClient.fetchState();
                        ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
                    }
            }
            break;

        case AppScreen::BRIGHTNESS:
            if (encoderMove != 0) {
                int bri = wledClient.getState().brightness;
                bri = constrain(bri + encoderMove * 25, 0, 255);
                wledClient.setBrightness(bri);
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
                    ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
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
                    ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
                }
            }
            break;

        default:
            break;
    }
}

void initAmbientSaver() {
    for (int i = 0; i < NUM_BLOBS; i++) {
        blobs[i].x = random(20, 150);
        blobs[i].y = random(20, 300);
        blobs[i].dx = (random(10, 30) / 100.0f) * (random(0,2) ? 1 : -1);
        blobs[i].dy = (random(10, 30) / 100.0f) * (random(0,2) ? 1 : -1);
        blobs[i].radius = random(10, 25);
        
        // Pick a soft color based on WLED color if available, or just random pastel
        uint8_t r = wledClient.getState().r;
        uint8_t g = wledClient.getState().g;
        uint8_t b = wledClient.getState().b;
        if (r < 20 && g < 20 && b < 20) {
            r = random(100, 255); g = random(100, 255); b = random(100, 255);
        }
        blobs[i].color = tft.color565(r/3, g/3, b/3); // dimmed
    }
}

void drawAmbientSaver() {
    uint32_t now = millis();
    if (now - lastBlobFrame < 50) return;  // ~20 fps
    lastBlobFrame = now;

    tft.fillScreen(TFT_BLACK);

    for (int i = 0; i < NUM_BLOBS; i++) {
        blobs[i].x += blobs[i].dx;
        blobs[i].y += blobs[i].dy;

        if (blobs[i].x < blobs[i].radius || blobs[i].x > 170 - blobs[i].radius) blobs[i].dx *= -1;
        if (blobs[i].y < blobs[i].radius || blobs[i].y > 320 - blobs[i].radius) blobs[i].dy *= -1;

        tft.fillCircle((int)blobs[i].x, (int)blobs[i].y, (int)blobs[i].radius, blobs[i].color);
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
                initAmbientSaver();
                saverInitialized = true;
            }
            drawAmbientSaver();
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

   void handleRecoveringState() {
        uint32_t now = millis();
        // Try to reconnect every 5 seconds (non-blocking)
        if (now - _recoverTimer >= 5000) {
             _recoverTimer = now;
             
             if (!wifiSetup.isConnected()) {
                 Serial.println("[WiFi] Reconnecting...");
                 _recoverMsg = "Reconnecting WiFi...";
                 ui.showRecovering(_recoverMsg);
                 if (wifiSetup.reconnect()) {
                     wledDiscovery.begin();
                     _recoverMsg = "Connecting WLED...";
                     ui.showRecovering(_recoverMsg);
                 }
             } else {
                 if (wledClient.fetchState()) {
                     transitionTo(AppState::RUNNING);
                 } else {
                     _recoverMsg = "Retrying WLED...";
                     ui.showRecovering(_recoverMsg);
                 }
             }
        }
    }

   // Handle loading state — wait for async fetch to complete with timeout
   static uint32_t _loadingStart = 0;
   static constexpr uint32_t LOADING_TIMEOUT_MS = 15000;

   void handleLoadingState() {
       uint32_t now = millis();

         // Start async fetch on first call
        if (wledClient.getFetchStatus() == FetchStatus::IDLE) {
             _loadingStart = now;
            wledClient.asyncFetchAll();
          }

         // Periodically redraw loading screen with animated dots
         static uint32_t _lastRender = 0;
        if (now - _lastRender >= 500) {
             _lastRender = now;
            ui.showLoading();
           }

         // Check completion or timeout
        if (wledClient.getFetchStatus() == FetchStatus::DONE) {
              wledClient.clearFetchStatus();
             transitionTo(AppState::RUNNING);
            } else if (now - _loadingStart >= LOADING_TIMEOUT_MS) {
                Serial.println("[WLED] Loading timeout, proceeding to RUNNING");
                 wledClient.clearFetchStatus();
                transitionTo(AppState::RUNNING);
               }
          }
