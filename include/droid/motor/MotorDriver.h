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
#include "droid/core/BaseComponent.h"

namespace droid::motor {
    class MotorDriver : public droid::core::BaseComponent {
    public:
        MotorDriver(const char* name, droid::core::System* system) :
            BaseComponent(name, system) {}
            
        //Virtual methods from BaseComponent redeclared here for clarity
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;

        virtual bool setMotorSpeed(uint8_t motor, int8_t speed) = 0;
        virtual bool arcadeDrive(int8_t joystickX, int8_t joystickY) = 0;
        virtual void stop() = 0;
    };
}