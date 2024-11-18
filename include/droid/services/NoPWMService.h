#pragma once
#include "PWMService.h"

namespace droid::services {
    class NoPWMService : public PWMService {
    public:
        NoPWMService(const char* name, droid::services::System* system) :
            PWMService(name, system) {}

        void init() override {}
        void factoryReset() override {}
        void task() override {}
        void logConfig() override {}
        void failsafe() override {}

        void setPWMuS(uint8_t outNum, uint16_t pulseMicroseconds, uint16_t durationMilliseconds = 0) override {}
        void setPWMpercent(uint8_t outNum, uint8_t percent, uint16_t durationMilliseconds = 0) override {}
        void setOscFreq(uint32_t freq) override {}
    };
}