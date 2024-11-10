#pragma once
#include <Arduino.h>

namespace droid::services {
    class PWMService {
    public:
        virtual void init() = 0;
        virtual void task() = 0;
        virtual void failsafe() = 0;
        virtual void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) = 0;
        virtual void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) = 0;
        virtual void setOscFreq(uint32_t freq) = 0;
    };
}