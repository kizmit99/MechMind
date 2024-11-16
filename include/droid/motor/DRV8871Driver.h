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

        bool setMotorSpeed(uint8_t motor, int8_t speed);
        bool arcadeDrive(int8_t joystickX, int8_t joystickY);
        void stop();

    private:
        void setMotorSpeed(int16_t speed);
        
        const char* name;
        droid::services::Logger* logger;
        droid::services::Config* config;
        droid::services::PWMService* pwmService;

        // Pin Numbers
        uint8_t out1;  // PWM output used for OUT1
        uint8_t out2;  // PWM output used for OUT2

        uint16_t timeoutMs = 100;
        uint8_t deadband = 0;
        float_t rampPowerPerMs = 0.1;
        ulong lastCommandMs = 0;
        ulong lastUpdateMs = 0;
        int16_t requestedDutyCycle = 0;
        int16_t currentDutyCycle = 0;
    };
}
