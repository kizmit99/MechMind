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
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;
        virtual void setCritical(bool isCritical) = 0;
        virtual void setDeadband(int8_t deadband) = 0;
        virtual int8_t getJoystickPosition(Joystick, Axis) = 0;
        virtual String getTrigger() = 0;
    };
}