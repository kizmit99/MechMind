#pragma once

#include <Arduino.h>

namespace droid::motor {
    class MotorDriver {
        virtual void factoryReset() = 0;
        virtual void task() = 0;
        virtual void drive(int8_t power) = 0;
        virtual void turn(int8_t power) = 0;
        virtual void stop() = 0;
        virtual void setTimeout(uint16_t milliSeconds) = 0;
    };
}