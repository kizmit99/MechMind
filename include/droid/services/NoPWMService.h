#pragma once
#include "PWMService.h"

namespace droid::services {
    class NoPWMService : public PWMService {
    public:
        NoPWMService() {}
        void init() {}
        void task() {}
        void failsafe() {}
        void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) {}
        void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) {}
        void setOscFreq(uint32_t freq) {}
    };
}