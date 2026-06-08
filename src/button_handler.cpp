#include "button_handler.h"

ButtonHandler buttons;

// Use a bare 5-pin mechanical rotary encoder:
//   - 3 pins for the rotary contacts (CLK/A, DT/B, GND common)
//   - 2 pins for the push switch (SW, GND)
// Internal INPUT_PULLUP is used for all encoder pins.
void ButtonHandler::begin(uint8_t leftPin, uint8_t rightPin,
                          uint8_t encoderClkPin, uint8_t encoderDtPin, uint8_t encoderSwPin) {
    _left.pin = leftPin;
    _right.pin = rightPin;
    _encoderButton.pin = encoderSwPin;
    _encoderClkPin = encoderClkPin;
    _encoderDtPin = encoderDtPin;
    _encoderSwPin = encoderSwPin;

    pinMode(leftPin, INPUT_PULLUP);
    pinMode(rightPin, INPUT_PULLUP);
    pinMode(encoderClkPin, INPUT_PULLUP);
    pinMode(encoderDtPin, INPUT_PULLUP);
    pinMode(encoderSwPin, INPUT_PULLUP);

    _encoderState = (digitalRead(encoderClkPin) << 1) | digitalRead(encoderDtPin);
    _encoderDelta = 0;
    _encoderMovement = 0;
}

void ButtonHandler::update() {
    _activity = false;
    updateButton(_left);
    updateButton(_right);
    updateButton(_encoderButton);
    updateEncoder();
}

void ButtonHandler::updateButton(ButtonState& btn) {
    bool reading = digitalRead(btn.pin);
    uint32_t now = millis();

    if (reading != btn.lastReading) {
        btn.lastDebounceTime = now;
    }
    btn.lastReading = reading;

    if ((now - btn.lastDebounceTime) < DEBOUNCE_MS) return;

    // Stable reading
    if (reading == LOW && !btn.pressed) {
        // Button just pressed
        btn.pressed = true;
        btn.pressStartTime = now;
        btn.longFired = false;
        _activity = true;
    } else if (reading == LOW && btn.pressed && !btn.longFired) {
        // Button held - check for long press
        if ((now - btn.pressStartTime) >= LONG_PRESS_MS) {
            btn.pendingEvent = ButtonEvent::LONG_PRESS;
            btn.longFired = true;
            _activity = true;
        }
    } else if (reading == HIGH && btn.pressed) {
        // Button released
        if (!btn.longFired) {
            btn.pendingEvent = ButtonEvent::SHORT_PRESS;
            _activity = true;
        }
        btn.pressed = false;
    }
}

void ButtonHandler::updateEncoder() {
    bool a = digitalRead(_encoderClkPin);
    bool b = digitalRead(_encoderDtPin);
    uint8_t newState = (a << 1) | b;

    if (newState != _encoderState) {
        static const int8_t transitionTable[16] = {
            0, -1, 1, 0,
            1, 0, 0, -1,
            -1, 0, 0, 1,
            0, 1, -1, 0
        };

        _encoderDelta += transitionTable[(_encoderState << 2) | newState];
        _encoderState = newState;

        if (_encoderDelta >= 4) {
            _encoderMovement += 1;
            _encoderDelta = 0;
            _activity = true;
        } else if (_encoderDelta <= -4) {
            _encoderMovement -= 1;
            _encoderDelta = 0;
            _activity = true;
        }
    }
}

ButtonEvent ButtonHandler::getLeftEvent() {
    ButtonEvent evt = _left.pendingEvent;
    _left.pendingEvent = ButtonEvent::NONE;
    return evt;
}

ButtonEvent ButtonHandler::getRightEvent() {
    ButtonEvent evt = _right.pendingEvent;
    _right.pendingEvent = ButtonEvent::NONE;
    return evt;
}

ButtonEvent ButtonHandler::getEncoderButtonEvent() {
    ButtonEvent evt = _encoderButton.pendingEvent;
    _encoderButton.pendingEvent = ButtonEvent::NONE;
    return evt;
}

int8_t ButtonHandler::getEncoderMovement() {
    int8_t movement = _encoderMovement;
    _encoderMovement = 0;
    return movement;
}

bool ButtonHandler::anyActivity() {
    return _activity;
}
