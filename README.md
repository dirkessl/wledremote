# WLED Remote+ - WLED Remote & HomeKit Bridge

A standalone ESP32-S3 controller for WLED that combines a physical remote UI with a HomeKit bridge on a LilyGo T-Display S3. The project provides local hardware controls, WLED discovery and control over the network, and Apple Home integration in a single device.

## Features

- Physical remote interface with the built-in display, left/right buttons, and rotary encoder on the LilyGo T-Display S3 hardware.
- WLED discovery on the local network using mDNS plus direct control through the WLED JSON API.
- HomeKit bridge support via HomeSpan, exposing WLED as a light plus preset switches for Apple Home.
- Captive-portal Wi-Fi onboarding with fallback configuration flow and persistent settings stored in NVS.
- Embedded local configuration web UI served on port 8080 for setup, discovery, status, and reset actions.
- Built with PlatformIO for the `lilygo-t-display-s3` environment and Arduino framework on ESP32.

## Hardware

The target device is a LilyGo T-Display S3 with ESP32-S3, PSRAM, a 170x320 ST7789 display, two front buttons, and a rotary encoder used for navigation and control.[cite:2] The display stack is driven with LovyanGFX, and the project also depends on WiFiManager, HomeSpan, and ArduinoJson.


## Setup

1. Flash the firmware to the board and boot it.[cite:1][cite:2]
2. Connect the device to Wi-Fi using the built-in WiFiManager flow; if saved credentials fail, it falls back to a captive portal for setup.[cite:2]
3. Discover a WLED instance on the local network or configure the WLED host manually through the local config UI.[cite:2]
4. Open the configuration page on port 8080 to review status, scan for devices, save settings, or reset configuration.[cite:2]
5. Pair the device with Apple Home using the HomeKit QR flow shown by the UI when HomeKit is enabled.[cite:2]

## HomeKit behavior

The HomeKit bridge exposes one light bulb accessory with On, Brightness, Hue, and Saturation characteristics, plus up to eight switch accessories that trigger WLED presets.[cite:2] The project overview also notes a default HomeKit setup code of `46637726`, and the firmware can generate an `X-HM://` pairing URI for QR-based setup.[cite:2]

## WLED integration

The firmware talks to WLED over HTTP using endpoints such as `/json/state`, `/json/effects`, and `/json/presets`, and it posts state updates back to `/json/state` for actions like power, brightness, color, effect, and preset selection.[cite:2] WLED instances are discovered over mDNS using the `_wled._tcp` service, which keeps first-time configuration simple on a local network.[cite:2]

## AI assistance

This project was developed with the help of AI-assisted tooling for planning, code generation, refactoring, and documentation support. Depending on the Task, different Technologies whre used. Mostly QWEN and GEMMA Models through ollama, but also cloud Models like Claude Opus were used.

## License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for the full text.

The project uses the following MIT/FreeBSD-licensed dependencies:
- **LovyanGFX** (FreeBSD)[web:19][web:34]
- **WiFiManager** (MIT)[web:20][web:32]
- **HomeSpan** (MIT)[web:29][web:35]
- **ArduinoJson** (MIT)[web:22][web:26]

All dependency licenses are permissive and compatible with the project's MIT license.[web:4][web:5]
