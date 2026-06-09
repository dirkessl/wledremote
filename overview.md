# WLED Remote & HomeKit Bridge — Project Overview

## What It Is

A standalone ESP32 device (LilyGo T-Display S3) that acts as:
1. **WLED Remote** — physical control for any WLED instance on the network via LCD + buttons + rotary encoder
2. **HomeKit Bridge** — exposes WLED as a HomeKit accessory (light bulb + preset switches), controllable from Apple Home

## Hardware

| Component | Detail |
|-----------|--------|
| Board | LilyGo T-Display S3 (ESP32S3, PSRAM) |
| Display | 170×320 ST7789, parallel 8-bit bus, PWM backlight on GPIO 38 |
| Buttons | Left (GPIO 0), Right (GPIO 14) — short & long press detected |
| Rotary Encoder | CLK: GPIO 21, DT: GPIO 22, SW: GPIO 23 — quadrature decoding with direction table |
| Display power | GPIO 15 (HIGH to enable) |
| Onboard LED | GPIO 2, used as HomeKit status pin |

## Libraries (lib_deps)

| Library | Purpose |
|---------|---------|
| LovyanGFX ^1.2.0 | Display driver (ST7789, 16-bit color, sprite support) |
| WiFiManager | Captive portal-based WiFi setup |
| HomeSpan ^2.1.8 | HomeKit Accessory Protocol implementation |
| ArduinoJson ^7.0.0 | JSON parsing for WLED REST API |

## Architecture

```
┌───────────────────────────────────────────────────┐
│                 main.cpp                            │
│   State machine (BOOT → WIFI_SETUP → ... → RUNNING)  │
│   Handles transitions & button routing               │
│   Screensaver with falling cat animations            │
│                                                      │
│   ┌───────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│   │ UI    │ │ Button   │ │ WLED     │ │ HomeKit  │  │
│   │       │ │ Handler  │ │ Client   │ │ Bridge   │  │
│   └──┬────┘ └──────────┘ └────┬─────┘ └──────────┘  │
│      │                       │                       │
│   ┌──▼─────┐            ┌────▼─────────┐            │
│   │ LGFX   │            │ Discovery    │            │
│   │ Setup  │            │ (mDNS)       │            │
│   └────────┘            └─────────────┘            │
│                                                      │
│   ┌───────────┐  ┌───────────┐  ┌──────────────┐    │
│   │ Config     │  │ WiFi      │  │ Config        │    │
│   │ Store (NVS)│  │ Setup     │  │ Server :8080  │    │
│   └───────────┘  └───────────┘  └──────────────┘    │
└──────────────────────────────────────────────────────┘
```

## Source Files

### Core

| File | Lines | Responsibility |
|------|-------|---------------|
| `main.cpp` | 683 | Entry point, state machine (6 states), button dispatch, screensaver logic with falling cat animations |
| `lgfx_setup.h` | 69 | LGFX init: parallel 8-bit bus, ST7789 panel (170×320), PWM backlight |

### UI Layer

| File | Lines | Responsibility |
|------|-------|---------------|
| `ui.h` / `ui.cpp` | ~890 combined | All screen rendering via LGFX sprite buffer. 13 screens: Boot, AP mode, Connecting, IP display, WLED scan/select, Main status (vertical slider), Menu, Color picker (12 swatches), Brightness, Presets/Effects lists, HomeKit pairing QR. iOS-style dark theme. Embedded MPS logo bitmap (80×80). |

### Input

| File | Lines | Responsibility |
|------|-------|---------------|
| `button_handler.h` / `.cpp` | ~180 combined | Rotary encoder quadrature decoding (4-transition threshold), 3-button debouncing (50ms), short/long press detection (800ms), activity flag for screensaver wake |

### WLED Communication

| File | Lines | Responsibility |
|------|-------|---------------|
| `wled_client.h` / `.cpp` | ~200 combined | HTTP client for WLED REST API. GET /json/state, /json/effects, /json/presets; POST /json/state (power, brightness, color via `seg[].col`, effect, preset). Parses segment array for RGB/fx/palette. |
| `wled_discovery.h` / `.cpp` | ~85 combined | mDNS discovery: queries `_wled._tcp`, populates WLEDDevice list. Registers self as "wled-remote" hostname. |

### HomeKit Bridge

| File | Lines | Responsibility |
|------|-------|---------------|
| `homekit_bridge.h` / `.cpp` | ~240 combined | Exposes WLED to Apple Home via HomeSpan. 1 LightBulb accessory (On, Brightness, Hue, Saturation) + up to 8 Switch accessories for presets (momentary). HSV↔RGB conversion helpers. Generates X-HM:// QR URI for pairing. Default code: 46637726. |

### Configuration & Networking

| File | Lines | Responsibility |
|------|-------|---------------|
| `config_store.h` / `.cpp` | ~100 combined | NVS Preferences under "wled-remote" namespace. Stores: WLED host/port, display brightness, HomeKit code. Factory reset support. |
| `wifi_setup.h` / `.cpp` | ~170 combined | WiFiManager integration — auto-connect with saved creds (2 attempts, 20s each), falls back to captive portal. Dual-credentials storage (WiFiManager NVS + own "wifi-creds" namespace). |
| `config_server.h` / `.cpp` | ~190 combined | Web UI on port 8080. Endpoints: GET /, POST /save, GET /scan (JSON), GET /status (JSON), POST /reset. Dark-themed single-page HTML for WLED config and device discovery. |

## State Machine Flow

```
BOOT → WIFI_SETUP → WIFI_CONNECTED ─┬─→ WLED_SCAN → WLED_SELECT → RUNNING
                                    │ (if no saved config)
                                    └─→ RUNNING (if WLED already configured)

  RUNNING loop:
    ├── Periodic poll (every 2s): fetchState() → syncHomeKit() → update UI
    ├── homeKitBridge.poll()
    ├── configServer.handleClient()
    └── handleButtons() → submenu navigation

  Menu items (from RUNNING):
    Colors    → color picker (12 preset swatches in 3×4 grid)
    Brightness→ dedicated brightness screen with progress bar
    Presets   → list of WLED presets, select to activate
    Effects   → list of WLED effects, select to activate
    HomeKit   → pairing code + QR code screen
    Settings  → shows IP, routes back to web config
```

## Screensaver

4-stage auto-dim after user inactivity (all timers configurable via `#define`):

| Stage | Duration | Behavior |
|-------|----------|----------|
| ON | 10s | Full brightness (200/255) |
| DIM | 10s | Reduced brightness (30/255) |
| SAVER | 10s | Falling cats animation (~20 fps, 8 drops, 4 face variants, pastel colors) |
| OFF | ∞ | Display off — any button activity wakes to ON |

## Key Parameters

| Parameter | Value |
|-----------|-------|
| Display resolution | 170×320 portrait |
| WLED poll interval | 2000ms |
| Encoder step threshold | 4 quadrature transitions |
| Button debounce | 50ms |
| Long press threshold | 800ms |
| HTTP timeout to WLED | 3000ms |
| Config server port | 8080 |
| HomeKit default code | 46637726 |
| Max preset switches in HomeKit | 8 |