/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/ActiveComponent.h"

namespace droid::controller {
    class Controller : public droid::core::ActiveComponent {
    public:
        enum Axis {
            X, Y};
        
        enum Joystick {
            LEFT, RIGHT};

        Controller(const char* name, droid::core::System* system) :
            ActiveComponent(name, system) {}
        
        //Virtual methods from ActiveComponent redeclared here for clarity
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