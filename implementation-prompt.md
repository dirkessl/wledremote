# Implementation Prompt for Coding Agents

Use this prompt as the direct instruction set for Claude Code, OpenCode, Codex, or another coding agent. This file is intentionally operational and task-oriented. The detailed product source of truth is `rebuild-spec.md`, and this prompt should be implemented in full alignment with it. [cite:8][cite:9]

## Role

You are rebuilding an existing embedded PlatformIO project into a polished, production-quality firmware for a **LilyGo T-Display S3**. Your job is to implement the firmware in this repository, not to design a different product. Preserve the current hardware platform, current library stack, and the modular structure of the codebase while improving reliability, UX, and maintainability. [cite:4][cite:7][cite:9]

## Primary Objective

Build a working **WLED Remote + minimal HomeKit bridge** with a modern dark Mushroom-card-inspired UI for the LilyGo T-Display S3. The device must control one selected WLED instance, reconnect robustly after Wi‑Fi/WLED interruptions, fetch preset names and effects from WLED, display them in the UI, allow them to be applied by encoder press, support a modern screensaver flow, and expose only On/Off plus Brightness to HomeKit. [cite:7][cite:8][cite:9]

## Read First

Before changing code:

1. Read `rebuild-spec.md` completely. [cite:8][cite:9]
2. Inspect the current project structure and preserve its modular decomposition around:
   - `main.cpp`
   - `button_handler.*`
   - `ui.*`
   - `wled_client.*`
   - `wled_discovery.*`
   - `wifi_setup.*`
   - `config_store.*`
   - `config_server.*`
   - `homekit_bridge.*`
   - `lgfx_setup.*` [cite:4][cite:7][cite:9]
3. Do not swap in different frameworks or unsupported hardware abstractions.

## Non-Negotiable Technical Constraints

- Keep PlatformIO target: `lilygo-t-display-s3`. [cite:3][cite:9]
- Keep Arduino framework on ESP32-S3. [cite:3][cite:9]
- Keep these libraries unless a genuine blocker makes it impossible:
  - LovyanGFX ^1.2.0
  - WiFiManager
  - HomeSpan ^2.1.8
  - ArduinoJson ^7.0.0 [cite:3][cite:7][cite:9]
- Keep the current hardware pin mapping:
  - Left button GPIO 0
  - Right button GPIO 14
  - Encoder CLK GPIO 21
  - Encoder DT GPIO 22
  - Encoder SW GPIO 23
  - Display enable GPIO 15
  - LED GPIO 2
  - Backlight PWM GPIO 38 [cite:2][cite:3][cite:7][cite:9]
- No touch support.
- No multilingual support.
- No battery features.
- No OTA.
- No Home Assistant-specific integration beyond controlling WLED itself. [cite:8][cite:9]

## Authoritative Product Behavior

### Main Control Mapping

In normal home mode:

- Rotate encoder = adjust **brightness only**.
- Press encoder = toggle WLED on/off.
- Left button = open menu.
- Right button = open presets. [cite:8][cite:9][conversation_history:1]

In menu mode:

- Rotate encoder = navigate menu items.
- Press encoder = enter/activate item.
- Right button = go back.
- Left button = go home. [cite:8][cite:9][conversation_history:1]

In preset mode:

- Rotate encoder = browse preset names.
- Press encoder = apply highlighted preset.
- Right button = go back.
- Left button = go home. [cite:8][cite:9][conversation_history:1]

Effects belong inside the Settings menu:

- Rotate encoder = browse effect names.
- Press encoder = apply highlighted effect.
- Highlighting must not apply anything until confirmed by press. [cite:8][cite:9][conversation_history:1]

No long-press features are required. [cite:8][conversation_history:1]

### Screen Wake and Idle Flow

- After 1 minute idle: dim display.
- After another 1 minute: start screensaver.
- After another 1 minute: turn display off. [cite:8][cite:9][conversation_history:1]
- Any activity wakes the UI immediately.
- On wake, refresh WLED state immediately. [cite:8][cite:9][conversation_history:1]
- Wake behavior should avoid accidental double-actions.

### WLED Availability

If WLED is offline, the UI must make that clear and continuously try to recover. The device should not appear healthy while disconnected from its selected WLED target. [cite:8][cite:9][conversation_history:1]

## Required UI Outcome

Implement a **modern dark TFT UI inspired by Home Assistant Mushroom cards**. [cite:8][cite:9][conversation_history:1]

The home screen must show:

- Large vertical brightness slider
- Brightness percentage text
- Current color reflected in the slider accent when available
- Small Wi‑Fi status indicator
- Small WLED connection status indicator
- Current preset name or current effect name [cite:8][cite:9][conversation_history:1]

The startup flow must be:

1. Centered logo screen
2. Connection status screen
3. Main screen immediately after both Wi‑Fi and WLED are connected [cite:8][cite:9][conversation_history:1]

Use subtle motion only. Do not reuse the old falling-cat aesthetic. The screensaver must feel modern and calm. [cite:7][cite:8][cite:9][conversation_history:1]

## Required Architecture

Implement or preserve these responsibilities:

- `main.cpp`: state machine, app lifecycle, input routing, polling schedule, wake/sleep behavior
- `button_handler.*`: button debounce, encoder decode, activity detection
- `ui.*`: pure rendering/state presentation helpers, no direct networking logic
- `wled_client.*`: REST calls and JSON parsing for state/effects/presets and state changes
- `wled_discovery.*`: mDNS discovery and last-target selection support
- `wifi_setup.*`: WiFiManager integration, reconnect management, captive portal flow
- `config_store.*`: NVS persistence
- `config_server.*`: local web configuration UI on port 8080
- `homekit_bridge.*`: minimal HomeKit light accessory
- `lgfx_setup.*`: display init, backlight, power enable, drawing foundation [cite:4][cite:7][cite:8][cite:9]

## Required Network Behavior

### Wi‑Fi

- Auto-connect using saved credentials.
- If connection is lost, retry automatically in the background.
- If the screen is on, dimmed, or in screensaver mode, show reconnect status in the UI. [cite:8][cite:9][conversation_history:1]
- Let the user choose whether to keep retrying or open captive portal when setup is needed. [cite:8][cite:9][conversation_history:1]
- Do not reboot-loop on network errors.

### WLED Device Selection

- Support one active WLED instance at a time.
- Discover devices using mDNS.
- Persist the last selected target.
- On boot and after reconnect, try the last selected target first. [cite:7][cite:8][cite:9][conversation_history:1]

### WLED API

Use and support at minimum:

- `GET /json/state`
- `GET /json/effects`
- `GET /presets.json`
- `POST /json/state` [cite:7][cite:8][cite:9]

Fetch and track:

- Power state
- Brightness
- Current color from primary segment if available
- Current effect
- Current preset if inferable
- Preset names
- Effect names
- Reachability / last sync status [cite:7][cite:8][cite:9]

### Effects and Presets

- Presets screen must show preset names fetched from WLED. [cite:8][cite:9][conversation_history:1]
- Effects screen must live under Settings and show effect names fetched from WLED. [cite:8][cite:9][conversation_history:1]
- Cache both lists for temporary offline visibility. [cite:8][cite:9][conversation_history:1]
- If cached data is shown, indicate that it may be stale.
- Applying preset/effect must require encoder press.

## Required HomeKit Behavior

Keep HomeKit as a required feature, but simplify it to a minimal lightbulb bridge.

Expose one HomeKit light with:

- On
- Brightness [cite:8][cite:9][conversation_history:1]

Do not require hue, saturation, or preset-switch accessories for this rebuild. The project may keep internal room for expansion, but ship the simpler model. [cite:7][cite:8][cite:9]

Include HomeKit reset in config functionality. [cite:8][cite:9][conversation_history:1]

## Required Config and Persistence

Persist at least:

- Last selected WLED target
- Wi‑Fi-related metadata where needed
- Cached preset names
- Cached effect names
- Hostname
- Display-related settings
- HomeKit reset/config state [cite:8][cite:9]

Important: do **not** treat cached WLED state as authoritative after boot. Always fetch live WLED state when connected. [cite:8][cite:9][conversation_history:1]

## Required Config Server

Retain a local config server on port 8080 with at least:

- Status page
- Wi‑Fi setup / portal flow trigger
- WLED target selection
- HomeKit reset
- Hostname edit [cite:7][cite:8][cite:9][conversation_history:1]

## Polling and Recovery Rules

Implement adaptive polling:

- More frequent while display is on
- Still fairly frequent while dimmed
- Slower in screensaver
- Periodic while display is off
- Immediate poll on wake [cite:8][cite:9][conversation_history:1]

Implement resilient reconnect scheduling for both Wi‑Fi and WLED without freezing UI rendering.

## Coding Rules

- Prefer small, maintainable classes/functions.
- Use explicit state enums.
- Prefer `millis()` scheduling over blocking delays.
- Avoid network calls inside rendering code.
- Avoid giant all-in-one functions.
- Avoid breaking the UI because of network latency.
- Keep serial logging useful but not excessively noisy. [cite:8][cite:9]

## Suggested Work Plan

1. Read and understand current codebase and `rebuild-spec.md`. [cite:4][cite:8][cite:9]
2. Refactor/clean module boundaries if needed without changing the external product behavior.
3. Implement reliable app state machine for boot, connect, run, recover, dim, screensaver, display-off.
4. Implement robust Wi‑Fi and WLED reconnect handling.
5. Implement WLED state/effects/presets fetch and cache behavior.
6. Implement home screen UI and menu/preset/effects flows.
7. Implement screensaver and wake behavior.
8. Simplify and verify HomeKit bridge for On/Brightness only.
9. Verify config server features.
10. Test acceptance criteria from `rebuild-spec.md` one by one. [cite:8][cite:9]

## Definition of Done

You are not done until all of these are true:

- Project builds successfully in PlatformIO for the LilyGo T-Display S3. [cite:3][cite:8][cite:9]
- Device boots through the required logo → connection → main-screen flow. [cite:8][cite:9][conversation_history:1]
- Encoder and buttons match the specified control mapping exactly. [cite:8][cite:9][conversation_history:1]
- Wi‑Fi reconnects automatically and visibly.
- WLED reconnects automatically and visibly.
- Last selected WLED target is restored.
- Presets and effects are fetched, displayed, cached, and applied only on encoder press. [cite:8][cite:9][conversation_history:1]
- Home screen shows the required modern status layout. [cite:8][cite:9][conversation_history:1]
- Idle dim → screensaver → display off works to the specified timings. [cite:8][cite:9][conversation_history:1]
- HomeKit controls On and Brightness successfully. [cite:8][cite:9][conversation_history:1]
- Config server supports Wi‑Fi, WLED target, HomeKit reset, and hostname configuration. [cite:8][cite:9][conversation_history:1]
- No out-of-scope features were added. [cite:8][cite:9][conversation_history:1]

## Final Instruction

Implement the firmware in this repository according to `rebuild-spec.md`. Make pragmatic engineering decisions where the spec leaves room, but do not change the defined hardware, library choices, control model, reconnect expectations, or overall product scope. The finished result should feel like a polished dedicated smart-home appliance, not a prototype. [cite:8][cite:9]
