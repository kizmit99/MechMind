/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/MechMind
 */

#pragma once
#include "droid/core/BaseComponent.h"

namespace droid::controller {
    class Controller : public droid::core::BaseComponent {
    public:

        //Axis Orientation defined as:
        //  X - Positive = Right, Negative = Left
        //  Y - Positive = Forward, Negative = Reverse
        enum Axis {
            X, Y};
        
        enum Joystick {
            LEFT, RIGHT};

        //This is a hack to work around the constraint that some of the
        //  Controller implementations do not support multiple instantiation
        enum ControllerType {
            STUB, DUAL_SONY, DUAL_RING};

        Controller(const char* name, droid::core::System* system) :
            BaseComponent(name, system) {}
        
        //Virtual methods from BaseComponent redeclared here for clarity
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;

        virtual void setCritical(bool isCritical) = 0;
        //Joystick Position should be returned as a value between -100 and +100 for each axis
        virtual int8_t getJoystickPosition(Joystick, Axis) = 0;
        virtual String getTrigger() = 0;

        virtual ControllerType getType() = 0;
    };
}