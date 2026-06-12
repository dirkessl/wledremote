#include <Arduino.h>
#include <WiFi.h>

#include "button_handler.h"
#include "config_server.h"
#include "config_store.h"
#include "homekit_bridge.h"
#include "lgfx_setup.h"
#include "ui.h"
#include "wifi_setup.h"
#include "wled_client.h"
#include "wled_discovery.h"

#define BUTTON_LEFT_PIN 0
#define BUTTON_RIGHT_PIN 14
#define POWER_PIN 15
#define LED_PIN 2

#define BUTTON_MENU_PIN BUTTON_LEFT_PIN
#define BUTTON_BACK_PIN BUTTON_RIGHT_PIN
#define ENCODER_CLK_PIN 1
#define ENCODER_DT_PIN 17
#define ENCODER_SW_PIN 10

static LGFX tft;

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

// Predefined colors for color picker

struct PresetColor {
  uint8_t r, g, b;
};
static const PresetColor presetColors[] = {
    {255, 0, 0},    {0, 255, 0},   {0, 0, 255},   {255, 255, 0},
    {255, 0, 255},  {0, 255, 255}, {255, 128, 0}, {255, 255, 255},
    {255, 180, 50}, {128, 0, 255}, {0, 128, 255}, {255, 64, 128},
};

// Forward declarations
void transitionTo(AppState newState);
void handleRunningState();
void handleRecoveringState();
void handleLoadingState();
void handleButtons();

// WiFi reconnect state
static uint8_t _wifiLostRetry = 0;
static String _recoverMsg = "";
static uint32_t _recoverTimer = 0;
static bool _homeKitStarted = false;
static bool _brightnessModeActive = false;
static int _previewBrightness = -1;
static bool _brightnessDirty = false;
static uint32_t _lastBrightnessInputMs = 0;
static constexpr uint32_t BRIGHTNESS_INTERACTION_HOLD_MS = 450;


uint32_t getPollInterval() {
  if (appState != AppState::RUNNING)
    return 5000;
  return 3000;
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
  tft.setRotation(0); // Portrait, buttons at bottom
  tft.setBrightness(200);
  tft.fillScreen(TFT_BLACK);

  // Initialize UI with display
  ui.begin(&tft);
  ui.showBoot();

  // Initialize buttons and rotary encoder
  buttons.begin(BUTTON_MENU_PIN, BUTTON_BACK_PIN, ENCODER_CLK_PIN,
                ENCODER_DT_PIN, ENCODER_SW_PIN);

  // Initialize config store (NVS)
  configStore.begin();

  // Load cached wled data
  wledClient.loadCache();

  // Onboard LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  delay(1500); // Show boot screen


  // Transition to WiFi setup
  transitionTo(AppState::WIFI_CONNECTING);
}

void loop() {
  // Always update buttons
  buttons.update();


  if (buttons.anyActivity()) {
    ui.resetActivityTimer();
  }

  // State machine
  switch (appState) {
  case AppState::BOOT:
    break;

  case AppState::WIFI_CONNECTING:
    // Config server running if connected, but WiFi manager handles portal
    break;

  case AppState::CAPTIVE_PORTAL:
    wifiSetup.processPortal();
    if (wifiSetup.isConnected()) {
      wledDiscovery.begin();
      configServer.begin();
      transitionTo(AppState::WLED_CONNECTING);
    } else if (!wifiSetup.isPortalActive()) {
      // Portal timed out
      ESP.restart();
    }
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
    Serial.println("[STATE] Captive Portal");
    ui.showAPMode("WLED-Bridge");
    configServer.stop();
    delay(100);
    wifiSetup.startPortal("WLED-Bridge");
    break;
  }

  case AppState::WLED_CONNECTING: {
    Serial.println("[STATE] WLED Connecting");
    ui.showWledConnecting();

    if (configStore.hasWLEDConfig()) {
      String host = configStore.getWLEDHost();
      uint16_t port = configStore.getWLEDPort();
      wledClient.setHost(host, port);

      if (wledClient.requestStateRefresh()) {
        transitionTo(AppState::LOADING);
      } else {
        _recoverMsg = "WLED busy/unreachable";
        transitionTo(AppState::RECOVERING);
      }
    } else {
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
      ui.showIP(wifiSetup.getIPAddress());
      ui.showMessage("No WLED Found",
                     "Use web UI to\nconfigure manually:\n" +
                         wifiSetup.getIPAddress());
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
    ui.showLoading();
    break;
  }

  case AppState::RUNNING: {
    Serial.println("[STATE] Running");

    bool wifiReallyUp =
        wifiSetup.isConnected() &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0);

    if (!_homeKitStarted && wifiReallyUp) {
      Serial.printf("[HomeKit] Starting on IP: %s\n",
                    WiFi.localIP().toString().c_str());
      homeKitBridge.begin(configStore.getHomeKitCode().c_str());
      _homeKitStarted = true;
      Serial.println("[HomeKit] Started");
    }

    ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
    lastPoll = millis();
    break;
  }

  case AppState::BOOT:
  default:
    break;
  }
}


void handleRunningState() {
  uint32_t now = millis();
  bool brightnessInteractionActive =
      _brightnessModeActive && ((now - _lastBrightnessInputMs) < BRIGHTNESS_INTERACTION_HOLD_MS);

  if (_brightnessModeActive && _brightnessDirty &&
      wledClient.getFetchStatus() != FetchStatus::IN_PROGRESS) {
    if (wledClient.setBrightness((uint8_t)_previewBrightness)) {
      _brightnessDirty = false;
    }
  }

  if (_brightnessModeActive && !brightnessInteractionActive &&
      !_brightnessDirty && wledClient.getFetchStatus() != FetchStatus::IN_PROGRESS &&
      ui.getCurrentScreen() == AppScreen::MAIN_STATUS) {
    _brightnessModeActive = false;
    _previewBrightness = -1;
    ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
  }

  // Poll WLED state periodically without blocking UI
  if (!_brightnessModeActive && now - lastPoll >= getPollInterval()) {
    lastPoll = now;
    if (wledClient.getFetchStatus() != FetchStatus::IN_PROGRESS) {
      wledClient.requestStateRefresh();
    }
  }

  FetchStatus fetchStatus = wledClient.getFetchStatus();
  if (fetchStatus == FetchStatus::DONE) {
    WLEDState confirmedState = wledClient.getState();
    wledClient.clearFetchStatus();
    homeKitBridge.syncState(confirmedState);

    if (_brightnessModeActive) {
      if (_previewBrightness >= 0 && confirmedState.brightness == (uint8_t)_previewBrightness) {
        _brightnessDirty = false;
      }
      if (ui.getCurrentScreen() == AppScreen::MAIN_STATUS && _previewBrightness >= 0) {
        WLEDState previewState = confirmedState;
        previewState.brightness = (uint8_t)_previewBrightness;
        ui.showMainStatus(previewState, wifiSetup.isConnected());
      }
    } else if (ui.getCurrentScreen() == AppScreen::MAIN_STATUS) {
      ui.showMainStatus(confirmedState, wifiSetup.isConnected());
    }
  } else if (fetchStatus == FetchStatus::FAILED) {
    wledClient.clearFetchStatus();
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
      encoderBtnEvt == ButtonEvent::NONE && encoderMove == 0)
    return;

  AppScreen screen = ui.getCurrentScreen();

  if (appState == AppState::RUNNING && screen == AppScreen::MAIN_STATUS) {
    if (encoderMove != 0) {
      WLEDState state = wledClient.getState();
      int baseBri = (_brightnessModeActive && _previewBrightness >= 0) ? _previewBrightness : state.brightness;
      int step = 8;
      int bri = constrain(baseBri + encoderMove * step, 0, 255);
      _brightnessModeActive = true;
      _previewBrightness = bri;
      _brightnessDirty = true;
      _lastBrightnessInputMs = millis();

      state.brightness = bri;
      ui.showMainStatus(state, wifiSetup.isConnected());
      return;
    }

    if (encoderBtnEvt == ButtonEvent::SHORT_PRESS) {
      _brightnessModeActive = false;
      _previewBrightness = -1;
      _brightnessDirty = false;
      wledClient.togglePower();
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
  if ((backEvt == ButtonEvent::SHORT_PRESS ||
       backEvt == ButtonEvent::LONG_PRESS) &&
      screen == AppScreen::RECOVERING) {
    transitionTo(AppState::CAPTIVE_PORTAL);
    return;
  }

  // Back from LOADING → skip back to running
  if ((backEvt == ButtonEvent::SHORT_PRESS ||
       backEvt == ButtonEvent::LONG_PRESS) &&
      (screen == AppScreen::LOADING)) {
    wledClient.clearFetchStatus();
    transitionTo(AppState::RUNNING);
    return;
  }

  // Back from submenu → main status
  if ((backEvt == ButtonEvent::SHORT_PRESS ||
       backEvt == ButtonEvent::LONG_PRESS) &&
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
      const WLEDDevice *device = wledDiscovery.getDevice(idx);
      if (device) {
        configStore.setWLEDHost(device->ip);
        configStore.setWLEDPort(device->port);
        wledClient.setHost(device->ip, device->port);
        if (wledClient.asyncFetchAll()) {
          transitionTo(AppState::LOADING);
        }
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
      case MenuItem::PRESETS:
        ui.showPresets(wledClient.getPresets());
        break;
      case MenuItem::EFFECTS:
        ui.showEffects(wledClient.getEffects());
        break;
      case MenuItem::HOMEKIT:
        ui.showHomeKitPairing(configStore.getHomeKitCode(),
                              homeKitBridge.getSetupURI());
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
        uint8_t bri = (_brightnessModeActive && _previewBrightness >= 0) ? (uint8_t)_previewBrightness : wledClient.getState().brightness;
        wledClient.setState(true, bri, presetColors[idx].r, presetColors[idx].g,
                            presetColors[idx].b);
        ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
      }
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
      const auto &presets = wledClient.getPresets();
      if (idx >= 0 && idx < (int)presets.size()) {
        wledClient.setPreset(presets[idx].first);
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
        ui.showMainStatus(wledClient.getState(), wifiSetup.isConnected());
      }
    }
    break;

  default:
    break;
  }
}

void handleRecoveringState() {
  if (_recoverMsg.length() > 0) {
    ui.showRecovering(_recoverMsg);
  }
}

void handleLoadingState() {
  FetchStatus status = wledClient.getFetchStatus();
  if (status == FetchStatus::DONE) {
    wledClient.clearFetchStatus();
    transitionTo(AppState::RUNNING);
    return;
  }
  if (status == FetchStatus::FAILED) {
    wledClient.clearFetchStatus();
    transitionTo(AppState::RUNNING);
    return;
  }
  ui.showLoading();
}

