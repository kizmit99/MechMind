#pragma once
#include <Arduino.h>

#include "droid/motor/MotorDriver.h"
#include "droid/services/System.h"

namespace droid::motor {
    class DRV8871Driver : public MotorDriver {
    public:
        DRV8871Driver(const char* name, droid::services::System* system, uint8_t out1, uint8_t out2);
        void init();
        void factoryReset();
        void task();
        void logConfig();
        void failsafe();
        void drive(int8_t power);
        void turn(int8_t power);
        void stop();
        void setTimeout(uint16_t milliSeconds);
        void setPowerRamp(float_t rampPowerPerMs);

    private:
        void setMotorSpeed(int16_t speed);
        
        const char* name;
        droid::services::Logger* logger;
        droid::services::PWMService* pwmService;

        // Pin Numbers
        uint8_t out1;  // PWM output used for OUT1
        uint8_t out2;  // PWM output used for OUT2

        uint16_t timeoutMs = 100;
        ulong lastCommandMs = 0;
        ulong lastUpdateMs = 0;
        float_t rampPowerPerMs = 0.1;
        int16_t requestedDutyCycle = 0;
        int16_t currentDutyCycle = 0;
    };
}
