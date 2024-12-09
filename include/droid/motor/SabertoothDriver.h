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
#include <motor/SabertoothDriver.h>

namespace droid::motor {
    class SabertoothDriver : public MotorDriver {
    public:
        SabertoothDriver(const char* name, droid::core::System* system, byte address, Stream* port);

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
        ::SabertoothDriver wrapped;
        Stream* port;

        int8_t lastMotorSpeed[2] = {0};
        unsigned long lastMotorUpdate[2] = {0};
    };
}