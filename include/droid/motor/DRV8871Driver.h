#pragma once
#include "droid/motor/MotorDriver.h"

namespace droid::motor {
    class DRV8871Driver : public MotorDriver {
    public:
        DRV8871Driver(const char* name, droid::core::System* system, uint8_t out1, uint8_t out2);

        //Override virtual methods from MotorDriver/ActiveComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        bool setMotorSpeed(uint8_t motor, int8_t speed);
        bool arcadeDrive(int8_t joystickX, int8_t joystickY);
        void stop();

    private:
        void setMotorSpeed(int16_t speed);
        
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
