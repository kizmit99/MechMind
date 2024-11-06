#pragma once
#include <Arduino.h>

namespace droid::motor {
    class MotorDriver {
    public:
        virtual void init() = 0;
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void logConfig() = 0;
        virtual void failsafe() = 0;
        virtual void drive(int8_t power) = 0;
        virtual void turn(int8_t power) = 0;
        virtual void stop() = 0;
        virtual void setTimeout(uint16_t milliSeconds) = 0;
        virtual void setPowerRamp(float rampPowerPerMs) = 0;
    };
}