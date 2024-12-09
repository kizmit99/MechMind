/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#pragma once
#include "droid/motor/MotorDriver.h"

namespace droid::motor {
    class DRV8871Driver : public MotorDriver {
    public:
        DRV8871Driver(const char* name, droid::core::System* system, int8_t M0_out1, int8_t M0_out2, int8_t M1_out1, int8_t M1_out2);

        //Override virtual methods from MotorDriver/BaseComponent
        void init() override;
        void factoryReset() override;
        void task() override;
        void logConfig() override;
        void failsafe() override;

        //Motor speed should be specified in a range from -100 to +100
        bool setMotorSpeed(uint8_t motor, int8_t speed);
        //Joystick Positions should be a value between -100 and +100 for each axis
        bool arcadeDrive(int8_t joystickX, int8_t joystickY);
        void stop();

    private:
        void setDutyCycle(uint8_t motor, int8_t speed);
        
        struct {
            // Pin Numbers
            int8_t out1;  // PWM output used for OUT1
            int8_t out2;  // PWM output used for OUT2

            uint16_t timeoutMs = 100;
            uint8_t deadband = 0;
            float_t rampPowerPerMs = 1.0;
            ulong lastCommandMs = 0;
            ulong lastUpdateMs = 0;
            int8_t requestedDutyCycle = 0;
            int8_t currentDutyCycle = 0;
        } motorDetails[2];
    };
}
