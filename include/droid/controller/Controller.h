#pragma once
#include <Arduino.h>

namespace droid::controller {
    class Controller {
    public:
        enum Axis {
            X, Y};
        
        enum Joystick {
            LEFT, RIGHT};
        
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual int8_t getJoystickPosition(Joystick, Axis) = 0;
        virtual String getTrigger() = 0;
    };
}