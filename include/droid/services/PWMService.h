#pragma once
#include "droid/core/ActiveComponent.h"

namespace droid::services {
    class PWMService : public droid::core::ActiveComponent {
    public:
        PWMService(const char* name, droid::core::System* system) :
            droid::core::ActiveComponent(name, system) {}

        //Virtual methods from ActiveComponent redeclared here for clarity
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;
        
        virtual void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) = 0;
        virtual void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) = 0;
        virtual void setOscFreq(uint32_t freq) = 0;
    };
}