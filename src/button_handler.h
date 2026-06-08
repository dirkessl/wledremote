#pragma once

#include <Arduino.h>
#include <functional>

enum class ButtonEvent {
    NONE,
    SHORT_PRESS,
    LONG_PRESS
};

class ButtonHandler {
public:
    void begin(uint8_t leftPin, uint8_t rightPin,
               uint8_t encoderClkPin, uint8_t encoderDtPin, uint8_t encoderSwPin);
    void update();

    ButtonEvent getLeftEvent();
    ButtonEvent getRightEvent();
    ButtonEvent getEncoderButtonEvent();
    int8_t getEncoderMovement();

    // Returns true if any button or encoder activity occurred
    bool anyActivity();

private:
    struct ButtonState {
        uint8_t pin = 0;
        bool lastReading = HIGH;
        bool stable = HIGH;
        uint32_t lastDebounceTime = 0;
        uint32_t pressStartTime = 0;
        bool pressed = false;
        bool longFired = false;
        ButtonEvent pendingEvent = ButtonEvent::NONE;
    };

    void updateButton(ButtonState& btn);
    void updateEncoder();

    ButtonState _left;
    ButtonState _right;
    ButtonState _encoderButton;
    bool _activity = false;

    uint8_t _encoderClkPin = 0;
    uint8_t _encoderDtPin = 0;
    uint8_t _encoderSwPin = 0;
    uint8_t _encoderState = 0;
    int8_t _encoderDelta = 0;
    int8_t _encoderMovement = 0;

    static constexpr uint32_t DEBOUNCE_MS = 50;
    static constexpr uint32_t LONG_PRESS_MS = 800;
};

extern ButtonHandler buttons;
