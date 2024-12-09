/*
 * MechMind Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/core/BaseComponent.h"

namespace droid::services {
    class PWMService : public droid::core::BaseComponent {
    public:
        PWMService(const char* name, droid::core::System* system) :
            droid::core::BaseComponent(name, system) {}

        //Virtual methods from BaseComponent redeclared here for clarity
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;
        
        virtual void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) = 0;
        virtual void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) = 0;
    };
}