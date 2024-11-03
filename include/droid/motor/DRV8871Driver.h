#pragma once

#include "droid/motor/MotorDriver.h"
#include "droid/services/System.h"

namespace droid::motor {
    class DRV8871Driver : public MotorDriver {
    public:
        DRV8871Driver(const char* name, droid::services::System* system, uint8_t pwm1, uint8_t pwm2);
        void factoryReset();
        void task();
        void drive(int8_t power);
        void turn(int8_t power);
        void stop();
        void setTimeout(uint16_t milliSeconds);
        void setDeadband(uint8_t deadband);
        void setPowerRamp(float_t rampPowerPerMs);

    private:
        void setMotorSpeed(int16_t speed);
        
        //instance name
        const char* name;
        droid::services::Logger* logger;

        // Pin Numbers
        uint8_t pwmPin1;  // PWM for OUT1
        uint8_t pwmPin2;  // PWM for OUT2

        uint16_t timeoutMs = 100;
        ulong lastCommandMs = 0;
        ulong lastUpdateMs = 0;
        uint8_t deadband = 3;
        float_t rampPowerPerMs = 0.1;
        int16_t requestedPWM = 0;
        int16_t currentPWM = 0;
    };
}
