#pragma once
#include <Arduino.h>

namespace droid::controller {
    class Controller {
    public:
        enum Axis {
            X, Y};
        
        enum Joystick {
            LEFT, RIGHT};
        
        virtual void task() = 0;
        virtual bool isButtonPressed(uint8_t buttonId) = 0;
        virtual bool isButtonClicked(uint8_t buttonId) = 0;
        virtual int8_t getJoystickPosition(Joystick, Axis) = 0;
        virtual String getTrigger() = 0;
    };
}