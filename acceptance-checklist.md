# Acceptance Checklist

Use this checklist after an AI coding agent has rebuilt the firmware. The goal is to verify that the rebuilt project matches `rebuild-spec.md` and `implementation-prompt.md` and behaves like the intended product rather than just compiling successfully. [cite:8][cite:10][cite:11]

## How to Use This Checklist

- Mark each item as:
  - `[ ]` Not verified
  - `[x]` Passed
  - `[!]` Failed / needs fix
  - `[-]` Not applicable
- Test on real hardware whenever possible.
- Prefer testing against a real WLED instance and a real HomeKit environment.
- Treat any mismatch in controls, reconnect behavior, or UI flow as a failure even if the firmware otherwise works. [cite:8][cite:10][cite:11]

## 1. Build and Project Integrity

- [ ] Project still uses PlatformIO and targets `lilygo-t-display-s3`. [cite:3][cite:8][cite:11]
- [ ] Project still uses Arduino framework on ESP32-S3. [cite:3][cite:8][cite:11]
- [ ] Project still uses the required library stack: LovyanGFX, WiFiManager, HomeSpan, ArduinoJson. [cite:3][cite:7][cite:8][cite:11]
- [ ] Project builds cleanly in PlatformIO without requiring a different board or framework. [cite:3][cite:8][cite:11]
- [ ] Source remains modular and includes the expected subsystems: main/app state, UI, buttons/encoder, WLED client, WLED discovery, Wi‑Fi setup, config store, config server, HomeKit bridge, display setup. [cite:4][cite:7][cite:8][cite:11]

## 2. Hardware Compatibility

- [ ] Left button uses GPIO 0. [cite:2][cite:8][cite:11]
- [ ] Right button uses GPIO 14. [cite:2][cite:8][cite:11]
- [ ] Encoder CLK uses GPIO 21. [cite:2][cite:8][cite:11]
- [ ] Encoder DT uses GPIO 22. [cite:2][cite:8][cite:11]
- [ ] Encoder switch uses GPIO 23. [cite:2][cite:8][cite:11]
- [ ] Display power enable uses GPIO 15. [cite:2][cite:8][cite:11]
- [ ] Onboard LED uses GPIO 2. [cite:2][cite:8][cite:11]
- [ ] Backlight PWM uses GPIO 38. [cite:7][cite:8][cite:11]
- [ ] Display renders correctly on the LilyGo T-Display S3 without orientation or clipping issues. [cite:3][cite:7][cite:8]

## 3. Boot and Startup Flow

- [ ] Device shows a centered logo on boot. [cite:8][cite:11][conversation_history:1]
- [ ] Device then shows a connection status screen. [cite:8][cite:11][conversation_history:1]
- [ ] Device enters the main screen as soon as both Wi‑Fi and WLED are connected. [cite:8][cite:11][conversation_history:1]
- [ ] Boot flow feels intentional and polished rather than flashing multiple debug-like screens. [cite:8][cite:11]

## 4. Main Screen UI

- [ ] Main screen uses a dark modern style inspired by Home Assistant Mushroom cards. [cite:8][cite:11][conversation_history:1]
- [ ] Main screen contains a large vertical brightness slider. [cite:8][cite:11][conversation_history:1]
- [ ] Main screen shows current brightness percentage text. [cite:8][cite:11][conversation_history:1]
- [ ] Main screen uses current WLED color as a slider accent when available. [cite:8][cite:11][conversation_history:1]
- [ ] Main screen shows a small Wi‑Fi status indicator. [cite:8][cite:11][conversation_history:1]
- [ ] Main screen shows a small WLED connection status indicator. [cite:8][cite:11][conversation_history:1]
- [ ] Main screen shows current preset name or current effect name. [cite:8][cite:11][conversation_history:1]
- [ ] UI updates smoothly without distracting or excessive animation. [cite:8][cite:11][conversation_history:1]

## 5. Home Mode Controls

- [ ] Rotating the encoder in home mode changes brightness only. [cite:8][cite:11][conversation_history:1]
- [ ] Pressing the encoder in home mode toggles WLED on/off. [cite:8][cite:11][conversation_history:1]
- [ ] Left button opens the menu from home mode. [cite:8][cite:11][conversation_history:1]
- [ ] Right button opens presets from home mode. [cite:8][cite:11][conversation_history:1]
- [ ] No long-press actions are required or exposed as part of normal product behavior. [cite:8][cite:11][conversation_history:1]

## 6. Menu Navigation

- [ ] In menu mode, the encoder rotates through menu items. [cite:8][cite:11][conversation_history:1]
- [ ] In menu mode, encoder press activates the highlighted item. [cite:8][cite:10][cite:11]
- [ ] In menu mode, right button goes back. [cite:8][cite:11][conversation_history:1]
- [ ] In menu mode, left button returns home. [cite:8][cite:11][conversation_history:1]
- [ ] Menu structure includes Settings and the required settings destinations such as Effects, Wi‑Fi/Network, WLED target, HomeKit, Hostname, Display/Screensaver, and About. [cite:8]

## 7. Presets Flow

- [ ] Right button opens a preset browser directly from home. [cite:8][cite:11][conversation_history:1]
- [ ] Preset browser shows preset **names** fetched from WLED. [cite:8][cite:11][conversation_history:1]
- [ ] In preset mode, encoder rotation moves through the list. [cite:8][cite:11][conversation_history:1]
- [ ] In preset mode, encoder press applies the highlighted preset. [cite:8][cite:11][conversation_history:1]
- [ ] Highlighting a preset does not apply it until encoder press. [cite:8][cite:11][conversation_history:1]
- [ ] In preset mode, right button goes back. [cite:8][cite:11][conversation_history:1]
- [ ] In preset mode, left button returns home. [cite:8][cite:11][conversation_history:1]

## 8. Effects Flow

- [ ] Effects are located inside Settings. [cite:8][cite:11][conversation_history:1]
- [ ] Effects screen shows names fetched from WLED. [cite:7][cite:8][cite:11]
- [ ] Encoder rotation moves through the effects list. [cite:8][cite:11][conversation_history:1]
- [ ] Encoder press applies the highlighted effect. [cite:8][cite:11][conversation_history:1]
- [ ] Highlighting an effect does not apply it automatically. [cite:8][cite:11][conversation_history:1]
- [ ] Right button goes back from effects. [cite:8][cite:10][cite:11]
- [ ] Left button returns home from effects. [cite:8][cite:10][cite:11]

## 9. Wi‑Fi Setup and Recovery

- [ ] Device auto-connects to saved Wi‑Fi credentials. [cite:7][cite:8][cite:11]
- [ ] If Wi‑Fi drops, the device retries automatically without crashing or reboot looping. [cite:8][cite:11][conversation_history:1]
- [ ] If the display is on, dimmed, or in screensaver, reconnect status is visible in the UI. [cite:8][cite:11][conversation_history:1]
- [ ] User can choose whether to keep retrying or open captive portal when setup/recovery is needed. [cite:8][cite:11][conversation_history:1]
- [ ] Captive portal flow remains available via WiFiManager. [cite:7][cite:8][cite:11]

## 10. WLED Discovery, Selection, and Recovery

- [ ] Device supports one active WLED target at a time. [cite:8][cite:11][conversation_history:1]
- [ ] Device discovers WLED instances using mDNS. [cite:7][cite:8][cite:11]
- [ ] Device persists the last selected WLED target. [cite:8][cite:11][conversation_history:1]
- [ ] On boot, device tries the last selected WLED target first. [cite:8][cite:11][conversation_history:1]
- [ ] If WLED becomes unavailable, the UI clearly shows recovery/reconnect activity. [cite:8][cite:11][conversation_history:1]
- [ ] Device continuously retries WLED reconnect instead of going idle silently. [cite:8][cite:11][conversation_history:1]
- [ ] Cached preset/effect data may still display while offline, but offline state is clearly indicated. [cite:8][cite:11]

## 11. WLED Data Handling

- [ ] Firmware fetches WLED state from `/json/state`. [cite:7][cite:8][cite:11]
- [ ] Firmware fetches effect names from `/json/effects`. [cite:7][cite:8][cite:11]
- [ ] Firmware fetches preset names from `/json/presets`. [cite:7][cite:8][cite:11]
- [ ] Firmware applies state changes via `POST /json/state`. [cite:7][cite:8][cite:11]
- [ ] Brightness, power, and visible UI state reflect the live WLED state when connected. [cite:8][cite:11]
- [ ] Cached values are not treated as authoritative on boot; live WLED state is fetched after reconnection. [cite:8][cite:11][conversation_history:1]

## 12. Polling and Wake Behavior

- [ ] Polling is more frequent while display is on. [cite:8][cite:11][conversation_history:1]
- [ ] Polling still occurs while display is dimmed. [cite:8][cite:11][conversation_history:1]
- [ ] Polling becomes slower during screensaver. [cite:8][cite:11][conversation_history:1]
- [ ] Polling still occurs periodically while display is off. [cite:8][cite:11][conversation_history:1]
- [ ] WLED state is refreshed immediately when display wakes. [cite:8][cite:11][conversation_history:1]
- [ ] Wake behavior does not feel laggy or cause accidental extra commands. [cite:8][cite:11]

## 13. Screensaver and Display Power

- [ ] Display dims after 1 minute of inactivity. [cite:8][cite:11][conversation_history:1]
- [ ] Screensaver starts after another 1 minute. [cite:8][cite:11][conversation_history:1]
- [ ] Display turns off after another 1 minute. [cite:8][cite:11][conversation_history:1]
- [ ] Screensaver looks modern and calm, not playful/cartoonish. [cite:8][cite:11][conversation_history:1]
- [ ] Old falling-cat screensaver is gone. [cite:7][cite:8][cite:11][conversation_history:1]
- [ ] Any activity wakes the display. [cite:8][cite:11][conversation_history:1]

## 14. HomeKit

- [ ] HomeKit remains a required feature in the rebuilt project. [cite:8][cite:11][conversation_history:1]
- [ ] HomeKit exposes a light accessory for the selected WLED target. [cite:8][cite:11]
- [ ] HomeKit supports On control. [cite:8][cite:11][conversation_history:1]
- [ ] HomeKit supports Brightness control. [cite:8][cite:11][conversation_history:1]
- [ ] Hue and Saturation are not required for acceptance. [cite:8][cite:11][conversation_history:1]
- [ ] HomeKit reset is available in config functionality. [cite:8][cite:11][conversation_history:1]

## 15. Config Server and Persistence

- [ ] Local config server still exists on port 8080. [cite:7][cite:8][cite:11]
- [ ] Config server shows current status. [cite:8][cite:11]
- [ ] Config server supports Wi‑Fi setup or captive portal trigger. [cite:8][cite:11]
- [ ] Config server supports WLED target selection/override. [cite:8][cite:11]
- [ ] Config server supports HomeKit reset. [cite:8][cite:11][conversation_history:1]
- [ ] Config server supports hostname editing. [cite:8][cite:11][conversation_history:1]
- [ ] Device persists last WLED target. [cite:8][cite:11][conversation_history:1]
- [ ] Device persists cached effect/preset names. [cite:8][cite:11][conversation_history:1]
- [ ] Device persists relevant display/UI settings. [cite:8][cite:11]

## 16. Performance and Stability

- [ ] UI remains responsive during Wi‑Fi reconnects. [cite:8][cite:11]
- [ ] UI remains responsive during WLED polling and command posts. [cite:8][cite:11]
- [ ] Screen drawing is smooth enough to feel polished on hardware. [cite:7][cite:8][cite:11]
- [ ] There is no obvious flicker, tearing, or ugly full-screen redraw behavior during normal use. [cite:8][cite:11]
- [ ] Serial logs are useful for diagnosis but not spammy. [cite:8][cite:11]
- [ ] Device runs stably for extended periods without lockups or progressive degradation. [cite:8][cite:11]

## 17. Out-of-Scope Guardrails

- [ ] No touch interface was added. [cite:8][cite:11][conversation_history:1]
- [ ] No multilingual UI was added. [cite:8][cite:11][conversation_history:1]
- [ ] No battery management features were added. [cite:8][cite:11][conversation_history:1]
- [ ] No OTA/update system was added as part of this rebuild scope. [cite:8][cite:11][conversation_history:1]
- [ ] No unrelated Home Assistant-specific functionality was added. [cite:8][cite:11][conversation_history:1]

## 18. Final Acceptance

- [ ] Rebuilt firmware behaves like a dedicated smart-home appliance, not a rough demo. [cite:8][cite:10][cite:11]
- [ ] Product behavior matches the defined control model exactly. [cite:8][cite:10][cite:11][conversation_history:1]
- [ ] Visual design matches the intended dark Mushroom-inspired direction well enough to be considered “beautiful, modern UI.” [cite:8][cite:10][cite:11][conversation_history:1]
- [ ] Reliability is good enough that temporary Wi‑Fi or WLED interruptions recover automatically in normal use. [cite:8][cite:10][cite:11][conversation_history:1]
- [ ] You would be comfortable handing this build to someone else as a polished version of the project. [cite:8][cite:10][cite:11]
