# WLED Remote Encoder Wiring

This project now supports a bare 5-pin rotary encoder plus the TTGO board buttons.

## Pin mapping

- `BUTTON_MENU_PIN = 0` — TTGO board button used to open the menu
- `BUTTON_BACK_PIN = 14` — TTGO board button used to go back
- `ENCODER_CLK_PIN = 21` — encoder CLK / A
- `ENCODER_DT_PIN = 22` — encoder DT / B
- `ENCODER_SW_PIN = 23` — encoder push-button
- `POWER_PIN = 15` — display power enable
- `LED_PIN = 2` — onboard LED

## Encoder wiring (bare 5-pin encoder)

A bare encoder typically has:
- 3 pins on one side for the rotary contacts
- 2 pins on the other side for the push switch

### Rotary side

    [CLK] [GND] [DT]

- `CLK` connects to `ENCODER_CLK_PIN` (GPIO 21)
- `DT` connects to `ENCODER_DT_PIN` (GPIO 22)
- `GND` connects to the board ground

### Switch side

    [SW] [GND]

- `SW` connects to `ENCODER_SW_PIN` (GPIO 23)
- `GND` connects to the same board ground

## Notes

- The code uses `INPUT_PULLUP` for `CLK`, `DT`, and `SW`.
- No external `VCC` connection is required for the encoder.
- If you want to confirm the pins on the encoder, use a continuity meter:
  - the two switch pins close when the encoder button is pressed
  - the rotary pins are the remaining three pins

## Operation

- In normal running mode:
  - rotate encoder = dim / brighten the light
  - press encoder = toggle WLED on/off
- TTGO board buttons:
  - menu button = open menu
  - back button = return to main status / exit submenu

If you want a diagram for a specific encoder model, send a photo or the pin orientation and I can adjust the mapping.