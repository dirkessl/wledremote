# WLED Remote Rebuild Specification

## Purpose

Rebuild this project as a production-quality PlatformIO firmware for the **LilyGo T-Display S3** that works as a dedicated **WLED remote** and **minimal HomeKit bridge**. The rebuilt project must preserve the current hardware platform and core library stack, improve reliability and UI quality, and be structured so an AI coding agent can implement it without guessing major behaviors. [cite:4][cite:7][conversation_history:1]

The end result must be a **working remote with a beautiful, modern, mushroom-card-style UI**, reliable Wi-Fi reconnect handling, WLED discovery and selection persistence, dynamic fetching of WLED presets and effects, a modern screensaver flow, and a simplified HomeKit accessory that supports only on/off and brightness. [cite:7][conversation_history:1]

## Non-Negotiable Constraints

- Keep **PlatformIO** project structure and target `lilygo-t-display-s3`. [cite:3][cite:7]
- Keep **Arduino framework** on ESP32-S3. [cite:3]
- Keep these libraries unless there is a proven build blocker:
  - `LovyanGFX ^1.2.0`
  - `WiFiManager`
  - `HomeSpan ^2.1.8`
  - `ArduinoJson ^7.0.0` [cite:3][cite:7]
- Keep the current hardware wiring and pin mapping:
  - Left button: GPIO 0
  - Right button: GPIO 14
  - Encoder CLK: GPIO 21
  - Encoder DT: GPIO 22
  - Encoder SW: GPIO 23
  - Display power enable: GPIO 15
  - Onboard LED: GPIO 2
  - TFT backlight PWM: GPIO 38 [cite:2][cite:3][cite:7]
- No touch support.
- No multilingual support.
- No battery-specific features.
- No Home Assistant integration beyond WLED itself.
- No extra scope such as OTA, buzzer/audio, or other ecosystem integrations. [conversation_history:1]

## Product Definition

The device is a dedicated tabletop or wall-mounted embedded controller for one selected WLED instance. It must:

1. Connect to Wi‑Fi using saved credentials or captive portal setup.
2. Discover available WLED devices on the network using mDNS.
3. Reconnect to the **last selected WLED instance** automatically after reboot or temporary network loss. [cite:7][conversation_history:1]
4. Render a polished TFT UI optimized for 170x320 portrait display.
5. Allow direct local control of WLED power and brightness.
6. Fetch **effects** and **preset names** from WLED and show them in the UI.
7. Apply a highlighted preset or effect only when confirmed by encoder press. [cite:7][conversation_history:1]
8. Expose the selected WLED instance to HomeKit with only **On** and **Brightness** characteristics required. [conversation_history:1]
9. Enter a dimmed idle state and then a screensaver, then switch the display off after inactivity. [conversation_history:1]

If WLED is offline, the device should not pretend to be fully operational. It must clearly show reconnect activity in the UI and continuously attempt recovery. [conversation_history:1]

## Hardware Specification

### Board

- Board: **LilyGo T-Display S3** with ESP32-S3 and PSRAM. [cite:3][cite:7]
- Display: ST7789 170x320, parallel 8-bit bus via LovyanGFX. [cite:3][cite:7]
- Use existing display-power and backlight control model from current project. [cite:2][cite:7]

### Input Devices

- Two board buttons with pull-ups and debounce.
- One bare 5-pin mechanical rotary encoder with quadrature decoding and push switch using internal pull-ups. [cite:2][cite:5][cite:6]
- Keep encoder handling robust against bounce and direction errors, using the same general approach as the current code: debounce for buttons, transition-table quadrature decode, movement accumulation thresholding. [cite:5][cite:6]

### Activity/Wake

- Any button press, encoder movement, or encoder push must count as activity.
- Any activity while screen is dimmed, in screensaver, or display-off state must wake the UI immediately.
- Waking the UI must not accidentally trigger a second action; the first event may be consumed as wake-only if necessary for a cleaner UX.

## Software Architecture

The rebuilt codebase should preserve the modular shape of the current project, which already contains `main`, button handling, UI, Wi‑Fi setup, config storage, config server, WLED client, WLED discovery, and HomeKit bridge modules. [cite:4][cite:7]

### Required Source Modules

Organize the code with the following responsibilities. Names can remain the same or be improved slightly, but the split of concerns must remain explicit:

- `main.cpp` — top-level app state machine, lifecycle coordination, timers, wake/sleep policy, polling scheduler, routing of input events to the current screen or mode.
- `lgfx_setup.*` — display initialization, backlight control, power enable, sprite creation, color helpers, font setup.
- `button_handler.*` — debounced button events, encoder rotation decode, unified activity detection. [cite:5][cite:6]
- `ui.*` — all drawing code and screen/state rendering; no network logic inside rendering functions.
- `wled_client.*` — HTTP access to WLED REST API, JSON parsing, state fetching, effect list fetching, preset list fetching, command posting.
- `wled_discovery.*` — mDNS discovery of `_wled._tcp`, local hostname registration, scan result normalization.
- `wifi_setup.*` — Wi‑FiManager integration, reconnect supervision, captive portal entry, connection status reporting.
- `config_store.*` — persistent preferences in NVS.
- `config_server.*` — lightweight local web UI on port 8080 for configuration and maintenance.
- `homekit_bridge.*` — HomeSpan integration for a minimal lightbulb accessory.

### Design Rules

- No giant monolithic loop with embedded UI/network logic.
- No blocking HTTP calls during animation-heavy drawing if this can freeze the UI.
- No direct UI code inside WLED or Wi‑Fi modules.
- Prefer explicit app state enums and screen enums.
- Prefer `millis()`-based scheduling instead of long blocking delays.
- All polling and reconnection logic must be time-sliced and non-blocking wherever possible.

## App State Model

Implement an explicit high-level state machine.

### High-Level States

1. `BOOT`
2. `WIFI_CONNECTING`
3. `CAPTIVE_PORTAL`
4. `WLED_CONNECTING`
5. `RUNNING`
6. `RECOVERING`
7. `HOMEKIT_PAIRING` (only if pairing info must be shown)
8. `DISPLAY_DIMMED`
9. `SCREENSAVER`
10. `DISPLAY_OFF`

### State Rules

- `BOOT` shows centered logo and initializes hardware.
- Transition to `WIFI_CONNECTING` immediately after boot setup. [conversation_history:1]
- After Wi‑Fi is up, enter `WLED_CONNECTING` and try last selected WLED target first. [conversation_history:1]
- Once both Wi‑Fi and WLED are connected and initial state is fetched, go to `RUNNING`. [conversation_history:1]
- If Wi‑Fi drops or WLED becomes unreachable, enter `RECOVERING` but keep trying automatically. [conversation_history:1]
- If user explicitly chooses portal mode, enter `CAPTIVE_PORTAL`.
- Idle timers can overlay the running state with `DISPLAY_DIMMED`, `SCREENSAVER`, and `DISPLAY_OFF` behavior.

The device must not reboot just because Wi‑Fi or WLED is unavailable.

## Input Mapping

This is the authoritative control mapping.

### Normal Mode (`RUNNING` main screen)

- Encoder rotate: adjust WLED **brightness only**. [conversation_history:1]
- Encoder short press: toggle WLED power on/off. [conversation_history:1]
- Left button short press: open **Menu**. [conversation_history:1]
- Right button short press: open **Presets**. [conversation_history:1]
- No long-press features are required. [conversation_history:1]

### Menu Mode

- Encoder rotate: move selection through menu items. [conversation_history:1]
- Encoder short press: enter/activate highlighted item.
- Right button short press: go back one level. [conversation_history:1]
- Left button short press: jump directly to Home/main screen. [conversation_history:1]

### Preset Mode

- Encoder rotate: move through preset names. [conversation_history:1]
- Encoder short press: apply highlighted preset. [conversation_history:1]
- Right button short press: go back. [conversation_history:1]
- Left button short press: go home. [conversation_history:1]

### Effects Screen

Effects live under the **Settings** menu. [conversation_history:1]

- Encoder rotate: move through effect names.
- Encoder short press: apply highlighted effect. [conversation_history:1]
- Highlighting alone must **not** apply an effect. [conversation_history:1]
- Right button short press: go back.
- Left button short press: go home.

### Wake Behavior

- Any activity wakes the screen from dimmed/screensaver/off states.
- Wake should feel instant.
- If wake-on-input would cause accidental toggles, consume the wake event and require the next intentional action.

## UI/UX Specification

### Visual Direction

Use a **Home Assistant Mushroom card style** adapted to a 170x320 portrait TFT. [conversation_history:1]

### Visual Principles

- Dark theme by default.
- Rounded cards.
- Clean spacing and hierarchy.
- Soft contrast, not harsh neon.
- Subtle motion only.
- Contemporary smart-home aesthetic, not a retro embedded look. [conversation_history:1]

### Animation Style

- Only subtle transitions: card fades, list easing, soft slide transitions, state badge morphs.
- No busy animation loops outside screensaver.
- No cartoon or novelty visuals such as the current falling cats. [cite:7][conversation_history:1]

### Display Layout: Main Screen

The main screen must show at minimum:

1. A large **vertical HomeKit-style brightness slider**.
2. Current brightness percentage as text.
3. Current color as the slider accent/fill color when available from WLED state. [conversation_history:1]
4. Small Wi‑Fi status indicator.
5. Small WLED connection status indicator.
6. Current preset name or, if unavailable, current effect name. [conversation_history:1]

#### Main Screen Behavior

- Brightness changes by encoder should animate the slider smoothly.
- Remote WLED changes detected from polling should update the slider and labels.
- If the light is off, show the slider in an off/disabled visual state but retain last known brightness value if WLED reports it.
- If current color is known, use it carefully as an accent without making text illegible.

### Menu Structure

Minimum menu tree:

- Settings
  - Effects
  - Wi‑Fi / Network
  - WLED Target
  - HomeKit
  - Hostname
  - Display / Screensaver
  - About

### Presets Screen

- Opened directly by the right button from home. [conversation_history:1]
- Displays preset names fetched from WLED. [conversation_history:1]
- Supports scrolling long lists cleanly.
- Shows highlighted item clearly.
- Applying a preset should show immediate visual confirmation.

### Effects Screen

- Lives inside Settings. [conversation_history:1]
- Lists all effects fetched from `/json/effects`. [cite:7]
- Selection highlight must be lightweight and readable.
- Only encoder press applies the effect. [conversation_history:1]

### Status/Connection Screens

Implement dedicated polished screens for:

- Boot logo
- Wi‑Fi connecting
- Captive portal active
- WLED connecting
- Recovering / reconnecting
- HomeKit pairing info when needed

At startup, the user specifically wants: centered logo first, then a connection status screen, then the main screen as soon as Wi‑Fi and WLED are connected. [conversation_history:1]

## Screensaver / Power Management

Replace the old falling-cat screensaver with a modern screensaver. The current project overview explicitly references the old cat screensaver, and that must be removed. [cite:3][cite:7][conversation_history:1]

### Idle Timeline

- After **1 minute** of inactivity: dim the display. [conversation_history:1]
- After another **1 minute**: enter screensaver mode. [conversation_history:1]
- After another **1 minute**: turn the display off entirely. [conversation_history:1]

### Screensaver Style

Use a modern low-motion visual such as one of these:

- Floating soft gradient blobs with very slow movement,
- Minimal ambient particles,
- Abstract color ribbons based on current WLED color,
- Elegant clock/status ambient screen.

Pick one and implement it well. The screensaver should feel premium, calm, and modern. [conversation_history:1]

### Screensaver Behavior

- Poll WLED less often than in active mode, but still periodically. [conversation_history:1]
- Any user activity wakes the screen.
- On wake, refresh WLED state immediately. [conversation_history:1]

## Wi‑Fi and Network Behavior

### Wi‑Fi Setup

Use WiFiManager as in the current project. [cite:3][cite:7]

Requirements:

- Auto-connect with saved credentials at boot.
- If connection fails, allow the user to decide whether to keep retrying or open captive portal. [conversation_history:1]
- Captive portal must remain available as a user-invoked recovery path.
- Show connection status on the TFT while connecting or retrying. [conversation_history:1]

### Wi‑Fi Reconnect

This is a critical requirement.

- Detect Wi‑Fi disconnects reliably.
- Begin background reconnect attempts automatically. [conversation_history:1]
- If the display is on, dimmed, or in screensaver mode, show reconnect progress/status in the UI rather than silently failing. [conversation_history:1]
- If the display is off, continue reconnect attempts headlessly and surface status once woken.
- Avoid boot loops or full app resets.

### WLED Reconnect

- Persist the last selected WLED host/device and try that first after boot or reconnect. [conversation_history:1]
- If Wi‑Fi reconnects successfully, immediately resume WLED reconnection.
- If WLED is offline, keep retrying and clearly show that the device is reconnecting because the device has little purpose without WLED. [conversation_history:1]
- If the configured WLED instance no longer exists, allow manual reselection via config server or UI.

## WLED Integration Requirements

Implement a robust `wled_client` using the WLED JSON API.

### Endpoints

Use at minimum:

- `GET /json/state`
- `GET /json/effects`
- `GET /json/presets`
- `POST /json/state` [cite:7]

### Required Fetched Data

From WLED state, fetch and track at minimum:

- On/off state
- Brightness
- Current RGB color if available from first segment
- Current effect index/name if available
- Current preset ID/name if inferable
- Reachability/last successful sync time [cite:7]

### Presets

- Fetch preset list from WLED.
- Show **preset names** in the preset browser. [conversation_history:1]
- Cache preset list locally for temporary offline display. [conversation_history:1]
- Clearly indicate when cached data is being shown because WLED is offline.

### Effects

- Fetch effect list from WLED.
- Show effect names in the effects screen.
- Cache effect list locally for temporary offline display. [conversation_history:1]
- Applying an effect requires encoder press. [conversation_history:1]

### Brightness Control

- Encoder motion in home screen changes brightness only. [conversation_history:1]
- Post brightness changes promptly but rate-limit enough to avoid flooding HTTP when the encoder moves quickly.
- UI should remain responsive while brightness posts are in flight.

### Power Toggle

- Encoder press on home screen toggles WLED power on/off. [conversation_history:1]
- UI should optimistically update then reconcile with server state.

### Caching Strategy

Cache these in NVS or RAM-plus-NVS as appropriate:

- Last known WLED host/port
- Last fetched effect names
- Last fetched preset names
- Last known brightness for UI continuity
- Theme/display settings
- Hostname

Do **not** treat cached WLED operational state as authoritative on boot. Current live WLED state must be fetched when the device reconnects. [conversation_history:1]

## Polling Strategy

Polling frequency must adapt to power/display state. The user explicitly requested more frequent polling when the display is active and slower polling when off. [conversation_history:1]

### Polling Rules

- **Display on / active:** poll relatively often for responsive UI.
- **Display dimmed:** still poll often enough to keep state fresh. [conversation_history:1]
- **Screensaver:** poll at a reduced interval.
- **Display off:** poll periodically at a lower frequency. [conversation_history:1]
- **On wake:** perform an immediate poll/update. [conversation_history:1]
- **During reconnect/recovery:** use a separate retry schedule with backoff and status text updates.

Suggested implementation defaults:

- Active: 750 ms to 1500 ms state poll
- Dimmed: 1500 ms to 2500 ms
- Screensaver: 3000 ms to 5000 ms
- Display off: 5000 ms to 15000 ms
- Effects/presets refresh: on first connect, on target change, and manual refresh if needed

These values can be constants and adjusted during tuning.

## HomeKit Requirements

Keep HomeKit as a required feature, but simplify it.

### Required Accessory Model

Expose exactly one primary **LightBulb** accessory representing the selected WLED instance, with:

- On
- Brightness [conversation_history:1]

### Optional/Not Required

- Hue: not required
- Saturation: not required
- Preset switches: not required

If desired internally, keep implementation hooks for future expansion, but the rebuilt project should ship with only the required minimal accessory behavior. This is a deliberate simplification compared with the current project overview, which describes more extensive HomeKit exposure. [cite:7][conversation_history:1]

### HomeKit Reset

- Provide HomeKit reset from config server and/or UI settings. [conversation_history:1]
- Persist HomeKit setup appropriately.
- If pairing QR/setup info must be shown, make it a dedicated polished screen.

## Config Storage Requirements

Use NVS/Preferences as in the current project. [cite:7]

Persist at least:

- Wi‑Fi-related settings required by WiFiManager and any local metadata
- Last selected WLED target
- Hostname
- Display brightness preference
- Screensaver/display timeout settings if made configurable
- Cached preset names
- Cached effect names
- HomeKit configuration data/reset state
- Any UI preferences needed for consistency [conversation_history:1]

Factory reset support should clear local app config and offer selective reset where practical:

- Reset Wi‑Fi
- Reset WLED target
- Reset HomeKit
- Full factory reset

## Config Server Requirements

Retain the local web server on port **8080**, consistent with the current architecture. [cite:3][cite:7]

### Required Config Functions

- Show current device/network/WLED status
- Configure Wi‑Fi or launch captive portal
- Choose or override WLED target
- Reset HomeKit
- Edit hostname [conversation_history:1]
- Trigger reboot if needed

### Nice-to-Have UX

- Clean single-page dark UI
- Show discovered WLED devices
- Save status feedback without requiring serial monitor

## Logging and Diagnostics

The rebuilt project should be debuggable over serial without becoming noisy in normal operation.

Requirements:

- Structured serial logs for boot, Wi‑Fi connect/disconnect, WLED connect/disconnect, HomeKit status, preset/effect fetch, config saves.
- Compile-time or runtime log level selection preferred.
- Avoid spamming logs on every frame draw.

## Error Handling Requirements

### If Wi‑Fi is unavailable

- Show Wi‑Fi reconnecting status.
- Keep retrying.
- Let the user choose captive portal when appropriate. [conversation_history:1]

### If WLED is unavailable

- Show WLED reconnecting status prominently.
- Keep retrying because the device is not useful without WLED. [conversation_history:1]
- Preset/effect lists may fall back to cached data, but applying them should be blocked or clearly fail gracefully while offline.

### If HTTP/API fetch fails temporarily

- Preserve last known UI state where sensible.
- Mark stale data subtly.
- Retry without crashing or freezing the UI.

## Performance and UX Quality Goals

- Smooth, flicker-free drawing using LovyanGFX sprites where appropriate. [cite:7]
- Fast wake from dim/screensaver/off states.
- Responsive encoder navigation even during network operations.
- No obvious frame tearing or full-screen redraw artifacts if avoidable.
- Keep memory usage safe for long-running uptime.

## Acceptance Criteria

The rebuild is successful only if all of the following are true:

1. Firmware builds in PlatformIO for `lilygo-t-display-s3`. [cite:3]
2. Device boots to logo, then connection status, then main UI once Wi‑Fi and WLED are connected. [conversation_history:1]
3. Device reconnects to saved Wi‑Fi without manual intervention when possible.
4. Device reconnects to the last selected WLED instance automatically after reboot and after temporary network loss. [conversation_history:1]
5. Home screen shows a modern vertical brightness slider, current percentage, Wi‑Fi state, WLED state, and current preset/effect. [conversation_history:1]
6. Encoder press toggles power in home mode. [conversation_history:1]
7. Encoder rotation changes brightness only in home mode. [conversation_history:1]
8. Left button opens menu from home. [conversation_history:1]
9. Right button opens presets from home. [conversation_history:1]
10. In menu or preset screens, encoder rotates through items. [conversation_history:1]
11. In menu mode, right button goes back. [conversation_history:1]
12. In menu/preset/effects screens, left button returns home. [conversation_history:1]
13. Presets are fetched from WLED and displayed by name. [conversation_history:1]
14. Effects are fetched from WLED and displayed in Settings. [conversation_history:1]
15. Highlighting an effect or preset does not apply it; encoder press applies it. [conversation_history:1]
16. After 1 minute idle the screen dims, after another minute screensaver starts, after another minute display turns off. [conversation_history:1]
17. Any activity wakes the display and immediately refreshes WLED state. [conversation_history:1]
18. HomeKit works for On and Brightness control. [conversation_history:1]
19. Config server allows Wi‑Fi, WLED target, HomeKit reset, and hostname management. [conversation_history:1]
20. Project contains no out-of-scope features such as OTA, touch, multilingual UI, or battery management. [conversation_history:1]

## Implementation Notes for AI Coding Agent

When rebuilding this project, do not invent alternative hardware, different libraries, or a different control model. Follow the current module structure and reuse the documented capabilities of the current codebase, especially the existing use of LovyanGFX for the display, WiFiManager for networking, HomeSpan for HomeKit, ArduinoJson for WLED JSON parsing, and the current button/encoder pin mapping and debounce/rotation model. [cite:3][cite:5][cite:6][cite:7]

Prefer clear, maintainable embedded C++ with small modules and deterministic state handling. The UI should feel premium and modern, but reliability is more important than visual ambition. The final result must feel like a polished appliance, not a demo sketch. [cite:7][conversation_history:1]
